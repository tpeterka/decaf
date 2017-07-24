//---------------------------------------------------------------------------
//
// redistribute base class
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <iostream>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>

#include <bredala/transport/cci/redist_cci.h>
#include <bredala/transport/cci/msg.h>

using namespace std;

void read_uris(string filename, vector<string>& server_uris)
{
    struct stat buffer;

    while(stat(filename.c_str(), & buffer) != 0)
        usleep(100000); // 100ms

    ifstream file;
    file.open(filename);

    while(!file.eof())
    {
        string uri;
        file>>uri;
        //fprintf(stderr, "Reading URI: %s\n", uri.c_str());
        server_uris.push_back(uri);
    }

    //fprintf(stderr, "The client read %lu uris.\n", server_uris.size());
}

void write_uri(CommHandle task_com, string filename, char* uri)
{
    int rank, size;
    MPI_Comm_rank(task_com, &rank);
    MPI_Comm_size(task_com, &size);

    int uri_length = strlen(uri);

    if (rank == 0)
    {
        vector<string> uris;
        string local_uri(uri);
        uris.push_back(local_uri);

        // Getting the hostfile in the right order
        for (unsigned int i = 1; i < size ; i++)
        {
            MPI_Status status;
            MPI_Probe(i, 352, task_com, &status);

            string received_uri;
            int nbytes;
            MPI_Get_count(&status, MPI_BYTE, &nbytes);
            received_uri.resize(nbytes);
            MPI_Recv(&received_uri[0], nbytes, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, task_com, &status);

            //fprintf(stderr, "URI received: %s\n", received_uri.c_str());
            uris.push_back(received_uri);
        }

        // Writting all the URI to a temporary file
        // This is because the client will poll to access the file
        // And we don't want the client to read the file before
        // the server has written everything. We write the full
        ofstream file;
        stringstream ss;
        ss<<filename<<".tmp";
        file.open(ss.str());
        for (unsigned int i = 0; i < uris.size(); i++)
        {
            if(i == uris.size()-1)
                file<<uris[i];
            else
                file<<uris[i]<<endl;
        }
        file.close();
        int ret = rename(ss.str().c_str(), filename.c_str());
        if(ret != 0)
        {
            fprintf(stderr, "ERROR: Failed to rename the URIs file. Abording.\n");
            MPI_Abort(task_com, 1);
        }

    }
    else
        MPI_Send(uri, uri_length, MPI_BYTE, 0, 352, task_com);
}


void
decaf::
RedistCCI::init_connection_client(vector<string>&  server_uris)
{
    int ret;
    channels_prod_.resize(server_uris.size());
    for (unsigned int i = 0; i < server_uris.size(); i++)
    {
        /* initiate connect */
        ret = cci_connect(endpoint_prod_, server_uris[i].c_str(), "Hello World!", 12,
                  CCI_CONN_ATTR_RO, NULL, 0, NULL);
        if (ret)
        {
            fprintf(stderr,"Connection failed.\n");
            fprintf(stderr, "cci_connect() returned %s\n",
                cci_strerror(endpoint_prod_, (cci_status)ret));
            exit(EXIT_FAILURE);
        }

        // Polling for connection acceptance event
        int ret;
        cci_event_t *event;

        bool connected = false;

        while (!connected)
        {
            ret = cci_get_event(endpoint_prod_, &event);

            if (ret != CCI_SUCCESS)
            {
                if (ret != CCI_EAGAIN)
                {
                    fprintf(stderr, "cci_get_event() returned %s",
                        cci_strerror(endpoint_prod_,  (cci_status)ret));
                }
                continue;
            }
            else
            {
                switch (event->type)
                {
                case CCI_EVENT_CONNECT:
                {
                    if (event->connect.status == CCI_SUCCESS)
                    {
                        channels_prod_[i].connection = event->connect.connection;
                        indexes_prod_.emplace(channels_prod_[i].connection, i);
                        connected = true;
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: EVENT_CONNECT returned a NULL connection handler\n");
                        exit(1);
                    }
                    break;
                }
                default:
                    fprintf(stderr, "ignoring event type %d while polling for a connection\n",
                        event->type);
                }
                cci_return_event(event);
            }
        }
    }

    //fprintf(stderr, "Connections initialized.\n");
}


void
decaf::
RedistCCI::init_connection_server(int nb_connections)
{
    int ret;

    int connection_left = nb_connections;
    //fprintf(stderr, "Waiting for %d connection.\n", nb_connections);
    while (connection_left > 0)
    {
        cci_event_t* event;

        ret = cci_get_event(endpoint_con_, &event);
        if (ret != CCI_SUCCESS)
        {
            if (ret != CCI_EAGAIN)
            {
                fprintf(stderr, "cci_get_event() returned %s",
                    cci_strerror(endpoint_con_,  (cci_status)ret));
            }
            continue;
        }
        switch (event->type)
        {
        case CCI_EVENT_CONNECT_REQUEST:
        {
            //fprintf(stderr, "Reception of a connection request...\n");
            ret = cci_accept(event, NULL);
            if (ret != CCI_SUCCESS)
            {
                fprintf(stderr,
                    "cci_accept() returned %s",
                    cci_strerror(endpoint_con_,  (cci_status)ret));
            }

            cci_return_event(event);
            break;
        }
        case CCI_EVENT_ACCEPT:
        {
            //fprintf(stderr, "Server accepting a connection. Building the channel structure.\n");
            CCIchannel channel;
            channel.connection = event->accept.connection;

            // Store the channel information
            channels_con_.emplace(channel.connection, channel);

            cci_return_event(event);
            connection_left--;
            break;
        }
        default:
        {
            event_queue_con_.push_back(event);
            break;
        }
        }
    }
}

void
decaf::
RedistCCI::poll_event(RedistRole role, int64_t it, cci_event_t **event)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        if(!event_queue_prod_.empty())
        {
            // Checking the event from the queue if one is available with the correct it
            auto item = event_queue_prod_.begin();
            while (item != event_queue_prod_.end())
            {
                if ((*item)->type != CCI_EVENT_RECV)
                {
                    fprintf(stderr, "ERROR: Event other that RECV stored in the queue. Removing the event.\n");
                    auto next_item = event_queue_prod_.erase(item);
                    item = next_item;
                    continue;
                }

                msg_cci *msg = (msg_cci *)((*item)->recv.ptr);
                if ((msg->it != it))
                    item++;
                else
                {
                    *event = *item;
                    event_queue_prod_.erase(item);
                    return;
                }
            }
        }

        cci_event_t *new_event;
        while(1)
        {
            int ret = cci_get_event(endpoint_prod_, &new_event);
            if (ret != CCI_SUCCESS)
            {
                if (ret != CCI_EAGAIN)
                {
                    fprintf(stderr, "cci_get_event() returned %s",
                        cci_strerror(endpoint_prod_,  (cci_status)ret));
                }
                continue;
            }
            else
            {
                switch (new_event->type)
                {
                case CCI_EVENT_RECV:
                {
                    msg_cci *msg = (msg_cci *)new_event->recv.ptr;
                    if ((msg->it != it))
                    {
                        event_queue_prod_.push_back(new_event);
                    }
                    else
                    {
                        *event = new_event;
                        return;
                    }
                    break;
                }
                case CCI_EVENT_SEND:
                {
                    *event = new_event;
                    return;
                }
                default:
                {
                    fprintf(stderr, "ERROR: unexpected event received in poll_event. Dropping the event.\n");
                    cci_return_event(new_event);
                    break;
                }
                }
            }
        }

    }
    else if (role == DECAF_REDIST_DEST)
    {
        if(!event_queue_con_.empty())
        {
            // Checking the event from the queue if one is available with the correct it
            auto item = event_queue_con_.begin();
            while (item != event_queue_con_.end())
            {
                if ((*item)->type != CCI_EVENT_RECV)
                {
                    fprintf(stderr, "ERROR: Event other that RECV stored in the queue. Removing the event.\n");
                    auto next_item = event_queue_con_.erase(item);
                    item = next_item;
                    continue;
                }

                msg_cci *msg = (msg_cci *)((*item)->recv.ptr);
                if ((msg->it != it))
                    item++;
                else
                {
                    *event = *item;
                    event_queue_con_.erase(item);
                    return;
                }
            }
        }

        // If we reach this point, that means that the queue is empty
        // or that none of the events in the queue has the correct it
        cci_event_t *new_event;
        while(1)
        {
            int ret = cci_get_event(endpoint_con_, &new_event);
            if (ret != CCI_SUCCESS)
            {
                if (ret != CCI_EAGAIN)
                {
                    fprintf(stderr, "cci_get_event() returned %s",
                        cci_strerror(endpoint_con_,  (cci_status)ret));
                }
                continue;
            }
            else
            {
                switch (new_event->type)
                {
                case CCI_EVENT_RECV:
                {
                    msg_cci *msg = (msg_cci *)new_event->recv.ptr;
                    // The case MSG_RMA_SENT cannot be handle for now because of the memory corruption
                    // Not problematic because the communication are ordered on the connection,
                    // Therefor if we receive the correct iteration for the mem_req, the next message
                    // on the connection will be for sure a RMA_SENT with the correct iteration and the
                    // client cannot perform a RMA operation without the server sending the memory handler first
                    if (msg->type == MSG_MEM_REQ && (msg->it != it))
                    {
                        event_queue_con_.push_back(new_event);
                    }
                    else
                    {
                        *event = new_event;
                        return;
                    }
                    break;
                }
                case CCI_EVENT_SEND:
                {
                    *event = new_event;
                    return;
                }
                default:
                {
                    fprintf(stderr, "ERROR: unexpected event received in poll_event. Dropping the event.\n");
                    cci_return_event(new_event);
                    break;
                }
                }
            }
        }

    }
    else
        fprintf(stderr, "ERROR: unknown RedistRole.\n");
}

// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistCCI::RedistCCI(int rankSource,
                     int nbSources,
                     int rankDest,
                     int nbDests,
                     int global_rank,
                     CommHandle communicator,
                     std::string name,
                     RedistCommMethod commMethod,
                     MergeMethod mergeMethod) :
    RedistComp(rankSource, nbSources, rankDest, nbDests, commMethod, mergeMethod),
    task_communicator_(communicator),
    sum_(NULL),
    destBuffer_(NULL),
    name_(name),
    endpoint_con_(nullptr),
    endpoint_prod_(nullptr),
    send_it(0),
    get_it(0)
{

    // For CCI components, we only have the communicator of the local task
    // The rank is the global rank in the application =! from the MPI redist components
    rank_ = global_rank;
    local_dest_rank_ = rankDest;
    local_source_rank_ = rankSource;

    fprintf(stderr, "Global rank: %d\n", rank_);

    MPI_Comm_rank(task_communicator_, &task_rank_);
    MPI_Comm_size(task_communicator_, &task_size_);
    int ret;
    cci_os_handle_t *os_handle = NULL;

    if(isSource() && isDest())
    {
        fprintf(stderr, "ERROR: overlapping case not implemented yet. Abording.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // group with all the sources
    if(isSource())
    {
        // Sanity check
        if(task_size_ != nbSources)
        {
            fprintf(stderr, "ERROR: size of the task communicator (%d) does not match nbSources (%d).\n", task_size_, nbSources);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Client side: Read all the URIs from the server side

        // Creating the local endpoint
        ret = cci_create_endpoint(NULL, 0, &endpoint_prod_, os_handle);

        if (ret)
        {
            fprintf(stderr, "cci_create_endpoint() failed with %s (%d)\n",
                cci_strerror(NULL,  (cci_status)ret), ret);
            exit(EXIT_FAILURE);
        }


        char *uri = NULL;
        ret = cci_get_opt(endpoint_prod_,
                  CCI_OPT_ENDPT_URI, &uri);
        if (ret)
        {
            fprintf(stderr, "cci_get_opt() failed with %s\n", cci_strerror(NULL,  (cci_status)ret));
            exit(EXIT_FAILURE);
        }

        // Read the URIs of the servers
        vector<string> uris;
        read_uris(name, uris);
        fprintf(stderr, "The component has read %lu uris.\n", uris.size());

        // Create a connection to each URI
        // Each producer create a connection with all the consumers
        init_connection_client(uris);

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        summerizeDest_ = new int[ nbDests_];
    }

    // group with all the destinations
    if(isDest())
    {

        // Sanity check
        if(task_size_ != nbDests)
        {
            fprintf(stderr, "ERROR: size of the task communicator (%d) does not match nbDests (%d).\n", task_size_, nbDests);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Server side: the generate all the URIs and write a file
        // for the clients

        // Creating the local endpoint
        ret = cci_create_endpoint(NULL, 0, &endpoint_con_, os_handle);

        if (ret)
        {
            fprintf(stderr, "cci_create_endpoint() failed with %s (%d)\n",
                cci_strerror(NULL,  (cci_status)ret), ret);
            exit(EXIT_FAILURE);
        }

        char *uri = NULL;
        ret = cci_get_opt(endpoint_con_,
                  CCI_OPT_ENDPT_URI, &uri);
        if (ret)
        {
            fprintf(stderr, "cci_get_opt() failed with %s\n", cci_strerror(NULL,  (cci_status)ret));
            exit(EXIT_FAILURE);
        }


        // Writing the URI of the all the destinations to a single file
        write_uri(communicator, name, uri);

        // Create all the connections between the local server and all the clients
        // We process only the event related to connections
        // Other events like received are put inside a queue
        init_connection_server(nbSources);
    }

    // TODO: maybe push that into redist_comp?
    destBuffer_ = new int[nbDests_];    // root consumer side: array sent be the root producer. The array is then scattered by the root consumer
    sum_ = new int[nbDests_];           // root producer side: result array after the reduce of summerizeDest. Member because of the overlapping case: The array need to be accessed by 2 consecutive function calls

    transit = pConstructData(false);

    // Make sure that only the events from the constructor are received during this phase
    // ie the producer cannot start sending data before all the servers created their connections
    MPI_Barrier(task_communicator_);

    // Everyone in the dataflow has read the file, the root clean the uri file
    if (isDest() && task_rank_ == 0)
        std::remove(name_.c_str());
}

decaf::
RedistCCI::~RedistCCI()
{
    // We don't clean the communicator, it was created by the app
    //if (task_communicator_ != MPI_COMM_NULL)
    //    MPI_Comm_free(&task_communicator_);

    // TODO: close the connections

    // Removing the file if we are the server
    if (isDest())
    {
        std::remove(name_.c_str());
    }

    delete[] destBuffer_;
    delete[] sum_;
}

void
decaf::
RedistCCI::splitSystemData(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        //if( summerizeDest_) delete [] summerizeDest_;
        //summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        // For this case we simply duplicate the message for each destination
        for(unsigned int i = 0; i < nbDests_; i++)
        {
            //Creating the ranges for the split function
            splitChunks_.push_back(data);

            //We send data to everyone except to self
            if(i + local_dest_rank_ != rank_)
                summerizeDest_[i] = 1;
            // rankDest_ - rankSource_ is the rank of the first destination in the
            // component communicator (communicator_)
            destList_.push_back(i + local_dest_rank_);

        }

        // All the data chunks are the same pointer
        // We just need to serialize one chunk
        if(!splitChunks_[0]->serialize())
            std::cout<<"ERROR : unable to serialize one object"<<std::endl;

    }
}

// redistribution protocol
void
decaf::
RedistCCI::redistribute(pConstructData& data, RedistRole role)
{
    switch(commMethod_)
    {
        case DECAF_REDIST_COLLECTIVE:
        {
            redistributeCollective(data, role);
            break;
        }
        case DECAF_REDIST_P2P:
        {
            redistributeP2P(data, role);
            break;
        }
        default:
        {
            std::cout<<"WARNING : Unknown redistribution strategy used ("<<commMethod_<<"). Using collective by default."<<std::endl;
            redistributeCollective(data, role);
            break;
        }
    }
}

// collective redistribution protocol
void
decaf::
RedistCCI::redistributeCollective(pConstructData& data, RedistRole role)
{
    fprintf(stderr, "ERROR: calling COLLECTIVE CCI\n");
    // TODO: to reimplement with CCI
/*    // debug
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // producer root collects the number of messages for each source destination
    if (role == DECAF_REDIST_SOURCE)
    {
        //for(int i = 0; i < nbDests_; i++)
        //    std::cout<<summerizeDest_[i]<<" ";
        //std::cout<<std::endl;
        MPI_Reduce(summerizeDest_, sum_, nbDests_, MPI_INT, MPI_SUM,
                   0, commSources_); // 0 Because we are in the source comm
                  //local_source_rank_, commSources_);
    }

    // overlapping source and destination
    if (rank_ == local_source_rank_ && rank_ == local_dest_rank_)
        memcpy(destBuffer_, sum_, nbDests_ * sizeof(int));

    // disjoint source and destination
    else
    {
        // Sending to the rank 0 of the destinations
        if(role == DECAF_REDIST_SOURCE && rank_ == local_source_rank_)
        {
            MPI_Request req;
            reqs.push_back(req);
            MPI_Isend(sum_,  nbDests_, MPI_INT,  local_dest_rank_, MPI_METADATA_TAG,
                      communicator_,&reqs.back());
        }


        // Getting the accumulated buffer on the destination side
        if(role == DECAF_REDIST_DEST && rank_ ==  local_dest_rank_) //Root of destination
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_METADATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_METADATA_TAG)  // normal, non-null get
            {
                MPI_Recv(destBuffer_,  nbDests_, MPI_INT, local_source_rank_,
                         MPI_METADATA_TAG, communicator_, MPI_STATUS_IGNORE);
            }

        }
    }

    // producer ranks send data payload
    if (role == DECAF_REDIST_SOURCE)
    {
        for (unsigned int i = 0; i <  destList_.size(); i++)
        {
            // sending to self, we store the data from the out to in
            if (destList_.at(i) == rank_)
                transit = splitChunks_.at(i);
            else if (destList_.at(i) != -1)
            {
                MPI_Request req;
                reqs.push_back(req);
                // nonblocking send in case payload is too big send in immediate mode
                MPI_Isend(splitChunks_.at(i)->getOutSerialBuffer(),
                          splitChunks_.at(i)->getOutSerialBufferSize(),
                          MPI_BYTE, destList_.at(i), send_data_tag, communicator_, &reqs.back());
            }
        }
        send_data_tag = (send_data_tag == INT_MAX ? MPI_DATA_TAG : send_data_tag + 1);
    }

    // check if we have something in transit to/from self
    if (role == DECAF_REDIST_DEST && !transit.empty())
    {
        if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
            data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
        else if (mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->unserializeAndStore(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
        transit.reset();              // we don't need it anymore, clean for the next iteration
        //return;
    }

    // only consumer ranks are left
    if (role == DECAF_REDIST_DEST)
    {
        // scatter the number of messages to receive at each rank
        int nbRecep;
        MPI_Scatter(destBuffer_, 1, MPI_INT, &nbRecep, 1, MPI_INT, 0, commDests_);

        // receive the payload (blocking)
        // recv_data_tag forces messages from different ranks in the same workflow link
        // (grouped by communicator) and in the same iteration to stay together
        // because the tag is incremented with each iteration
        for (int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, recv_data_tag, communicator_, &status);
            int nbytes; // number of bytes in the message
            MPI_Get_count(&status, MPI_BYTE, &nbytes);
            data->allocate_serial_buffer(nbytes); // allocate necessary space
            MPI_Recv(data->getInSerialBuffer(), nbytes, MPI_BYTE, status.MPI_SOURCE,
                     recv_data_tag, communicator_, &status);

            // The dynamic type of merge is given by the user
            // NOTE: examine if it's not more efficient to receive everything
            // and then merge. Memory footprint more important but allows to
            // aggregate the allocations etc
            if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                data->merge();
            else if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                //data->unserializeAndStore();
                data->unserializeAndStore(data->getInSerialBuffer(), nbytes);
            else
            {
                std::cout<<"ERROR : merge method not specified. Abording."<<std::endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        }
        recv_data_tag = (recv_data_tag == INT_MAX ? MPI_DATA_TAG : recv_data_tag + 1);

        if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->mergeStoredData();
    }
*/
}

// point to point redistribution protocol
void
decaf::
RedistCCI::redistributeP2P(pConstructData& data, RedistRole role)
{
    int ret;

    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {

        // First we send the request for the memory handler
        // and register the local memory
        for (unsigned int i = 0; i < channels_prod_.size(); i++)
        {
            if(splitChunks_[i].getPtr()) // Full message to send
            {
                // Sending the memory request
                msg_cci req_mem;
                req_mem.type = MSG_MEM_REQ;
                req_mem.it = send_it;
                req_mem.value = splitChunks_[i]->getOutSerialBufferSize();

                cci_send(channels_prod_[i].connection, &req_mem, sizeof(req_mem), (void*)1, 0);

                // Deregistering the previous memory if any
                if(channels_prod_[i].buffer_size > 0)
                    cci_rma_deregister(endpoint_prod_, channels_prod_[i].local_rma_handle);

                // Registering the local memory
                int flags = 0; // The local memory won't be read or written by other endpoints
                ret = cci_rma_register(endpoint_prod_, splitChunks_[i]->getOutSerialBuffer(),
                                       splitChunks_[i]->getOutSerialBufferSize(), flags,
                                       &channels_prod_[i].local_rma_handle);
                channels_prod_[i].buffer_size = splitChunks_[i]->getOutSerialBufferSize();
            }
            else // Empty message still sent to unlock the server
            {
                msg_cci req_mem;
                req_mem.type = MSG_MEM_REQ;
                req_mem.it = send_it;
                req_mem.value = 1;

                cci_send(channels_prod_[i].connection, &req_mem, sizeof(req_mem), (void*)1, 0);

                // Deregistering the previous memory if any
                if(channels_prod_[i].buffer_size > 0)
                    cci_rma_deregister(endpoint_prod_, channels_prod_[i].local_rma_handle);

                int flags = 0;
                // We don't use the serial buffer from the pConstructData because the pointer is not initialized
                // Instead we use the buffer from the channel
                channels_prod_[i].buffer.resize(1);
                ret = cci_rma_register(endpoint_prod_, &channels_prod_[i].buffer[0],
                                       1, flags,
                                       &channels_prod_[i].local_rma_handle);
                channels_prod_[i].buffer_size = 1;
            }
        }

        int nb_send = channels_prod_.size();

        // Looping over the events
        // Protocol:
        // When receiving a distant RMA handler -> performing a RMA operation
        // When receiving a Ack from the server -> the send is complete
        while (nb_send > 0)
        {
            cci_event_t *event;

            poll_event(role, send_it, &event);

            switch (event->type)
            {
            case CCI_EVENT_RECV:
            {
                msg_cci *msg = (msg_cci *)event->recv.ptr;
                if (msg->type == MSG_OK)
                {
                    // The server is done processing the data
                    nb_send--;
                }
                else if (msg->type == MSG_MEM_HANDLE)
                {
                    //fprintf(stderr, "Reception of the remote memory handler. Sending the message...\n");
                    // Getting the server handle
                    unsigned int index_connection = indexes_prod_.at(event->recv.connection);
                    channels_prod_[index_connection].distant_rma_handle = (cci_rma_handle_t *)malloc(sizeof(cci_rma_handle_t));
                    memcpy((void*)channels_prod_[index_connection].distant_rma_handle, msg->rma_handle, sizeof(cci_rma_handle_t));
                    //fprintf(stderr, "Handler received: %llu %llu %llu %llu\n",
                    //        channels[index_connection].distant_rma_handle->stuff[0],
                    //        channels[index_connection].distant_rma_handle->stuff[1],
                    //        channels[index_connection].distant_rma_handle->stuff[2],
                    //        channels[index_connection].distant_rma_handle->stuff[3]);

                    // Starting the RMA operation
                    msg_cci ack_msg;
                    ack_msg.type = MSG_RMA_SENT;
                    ack_msg.it = send_it;

                    //fprintf(stderr, "Sending the ack type: %u\n", ack_msg.type);

                    ret = cci_rma(channels_prod_[index_connection].connection,
                                  &ack_msg, sizeof(msg_cci),
                                  channels_prod_[index_connection].local_rma_handle, 0,
                                  channels_prod_[index_connection].distant_rma_handle, 0,
                                  channels_prod_[index_connection].buffer_size,
                                  (void*)1, CCI_FLAG_WRITE);

                    if (ret)
                        fprintf(stderr, "cci_rma returned %s\n",
                            cci_strerror(endpoint_prod_, (cci_status)ret));
                }
                else
                {
                    fprintf(stderr, "Reception of an unknow message: %u\n", msg->type);
                }
                break;
            }
            case CCI_EVENT_SEND:
            {
                // Not doing anything
                break;
            }
            default:
                fprintf(stderr, "Unexptected event %d\n", event->type);
                break;
            }

            cci_return_event(event);
        }

        send_it++;
    }

    if(role == DECAF_REDIST_DEST)
    {
        int nbRecep;
        if(isSource()) //Overlapping case
            nbRecep = nbSources_-1;
        else
            nbRecep = nbSources_;
        // Number of send done with the server
        // Used to count the number of event EVENT_SENT we need to wait

        while (nbRecep > 0)
        {
            cci_event_t *event;
            poll_event(role, get_it, &event);

            switch (event->type)
            {

            case CCI_EVENT_RECV:
            {
                msg_cci *msg = (msg_cci *)event->recv.ptr;
                CCIchannel& channel = channels_con_.at(event->recv.connection);

                if (msg->type == MSG_MEM_REQ)
                {
                    // The client wants to send data. Allocating memory
                    //fprintf(stderr, "Reception of a memory request...\n");

                    // Deregistering the previous memory if required
                    if(channel.buffer_size > 0)
                        cci_rma_deregister(endpoint_con_, channel.local_rma_handle);


                    // Creating the memory buffer and handler
                    //ret = posix_memalign((void **)&(channel.buffer), 4096, msg->value);
                    //check_return(endpoint, "memalign buffer", ret, 1);
                    channel.buffer.resize(msg->value);
                    channel.buffer_size = msg->value;

                    memset(&channel.buffer[0], 'b', msg->value);
                    channel.buffer[msg->value-1] = '\n';

                    int flags = CCI_FLAG_WRITE;
                    ret = cci_rma_register(endpoint_con_, &channel.buffer[0], msg->value, flags,
                                           &channel.local_rma_handle);
                    //check_return(endpoint_con_, "cci_rma_register", ret, 1);

                    // Sending the local handler to the distant
                    msg_cci msg_handle;
                    msg_handle.type = MSG_MEM_HANDLE;
                    msg_handle.it = get_it;
                    memcpy(msg_handle.rma_handle, channel.local_rma_handle, sizeof(msg_handle.rma_handle));

                    ret = cci_send(channel.connection, &msg_handle, sizeof(msg_cci), (void*)1, CCI_FLAG_SILENT);

                }
                else if(msg->type == MSG_RMA_SENT)
                {
                    // The client has sent data in RDMA and the operation is complete.
                    //fprintf(stderr, "Memory state: %s\n", channel.buffer);

                    msg_cci msg_ack;
                    msg_ack.type = MSG_OK;
                    msg_ack.it = get_it;
                    cci_send(event->recv.connection, &msg_ack, sizeof(msg_cci), (void*)1, CCI_FLAG_BLOCKING);

                    // Empty message case
                    if(channel.buffer_size == 1)
                    {
                        nbRecep--;
                        cci_return_event(event);
                        continue;
                    }

                    // Full message case: merging the received msg
                    data->allocate_serial_buffer(channel.buffer_size);
                    memcpy(data->getInSerialBuffer(), &channel.buffer[0], channel.buffer_size);

                    // The dynamic type of merge is given by the user
                    // NOTE : examin if it's not more efficient to receive everything
                    // and then merge. Memory footprint more important but allows to
                    // aggregate the allocations etc
                    if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                        data->merge();
                    else if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                        //data->unserializeAndStore();
                        data->unserializeAndStore(data->getInSerialBuffer(), channel.buffer_size);
                    else
                    {
                        std::cout<<"ERROR : merge method not specified. Abording."<<std::endl;
                        MPI_Abort(MPI_COMM_WORLD, 0);
                    }

                    nbRecep--;
                }
                else
                {
                    // BUG: that corresponds to the message sent from the client
                    // at the end of the RDMA operation (MSG_RMA_SENT) but the content of the
                    // msg is corrupted.
                    // Matthieu: Seems to work on Froggy

                    msg_cci msg_ack;
                    msg_ack.type = MSG_OK;
                    msg_ack.it = get_it;
                    cci_send(event->recv.connection, &msg_ack, sizeof(msg_cci), (void*)1, CCI_FLAG_BLOCKING);

                    // Empty message case
                    if(channel.buffer_size == 1)
                    {
                        nbRecep--;
                        cci_return_event(event);
                        continue;
                    }

                    // Full message case: merging the received msg
                    data->allocate_serial_buffer(channel.buffer_size);
                    memcpy(data->getInSerialBuffer(), &channel.buffer[0], channel.buffer_size);

                    // The dynamic type of merge is given by the user
                    // NOTE : examin if it's not more efficient to receive everything
                    // and then merge. Memory footprint more important but allows to
                    // aggregate the allocations etc
                    if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                        data->merge();
                    else if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                        //data->unserializeAndStore();
                        data->unserializeAndStore(data->getInSerialBuffer(), channel.buffer_size);
                    else
                    {
                        std::cout<<"ERROR : merge method not specified. Abording."<<std::endl;
                        MPI_Abort(MPI_COMM_WORLD, 0);
                    }

                    nbRecep--;
                }

                break;
            }
            case CCI_EVENT_SEND:
            {
                //Not doing anything
                break;
            }

            default:
                fprintf(stderr, "unexpected event %d", event->type);
                break;
            }
            cci_return_event(event);
        }

        // Checking if we have something in transit
        if(!transit.empty())
        {
            if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
            else if (mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                data->unserializeAndStore(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());

            //We don't need it anymore. Cleaning for the next iteration
            transit.reset();
        }

        if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->mergeStoredData();

        get_it++;
    }

    // Make sure that we process all the requests of 1 iteration before receiving events from the next one
    // TODO: use the queue event to manage stored events belonging to other iterations
    // NOTE Matthieu: Implemented poll_event. Have to check on a cluster to see if it holds.
    //MPI_Barrier(task_communicator_);
}

void
decaf::
RedistCCI::flush()
{
    /*if (reqs.size())
        MPI_Waitall(reqs.size(), &reqs[0], MPI_STATUSES_IGNORE);
    reqs.clear();*/

    // data have been received, we can clean it now
    // Cleaning the data here because synchronous send.
    // TODO: move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

void
decaf::
RedistCCI::shutdown()
{
    /*for (size_t i = 0; i < reqs.size(); i++)
        MPI_Request_free(&reqs[i]);
    reqs.clear();*/

    // data have been received, we can clean it now
    // Cleaning the data here because synchronous send.
    // TODO: move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

void
decaf::
RedistCCI::clearBuffers()
{
    splitBuffer_.clear();
}

