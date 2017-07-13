#include <bredala/transport/cci/redist_proc_cci.h>

void
decaf::
RedistProcCCI::computeGlobal(pConstructData& data, RedistRole role)
{
    if( !initializedSource_ && role == DECAF_REDIST_SOURCE )
    {
        if(nbSources_ > nbDests_ && nbSources_ % nbDests_ != 0)
        {
            std::cerr<<"ERROR : the number of destination does not divide the number of sources. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        else if(nbSources_ < nbDests_ && nbDests_ % nbSources_ != 0)
        {
            std::cerr<<"ERROR : the number of sources does not divide the number of destinations. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        int source_rank;
        MPI_Comm_rank( task_communicator_, &source_rank);

        if(nbSources_ >= nbDests_) //N to M case (gather)
        {
            destination_ = floor(source_rank / (nbSources_ / nbDests_));
            nbSends_ = 1;
        }
        else // M to N case (broadcast)
        {
            nbSends_ = nbDests_ / nbSources_;
            destination_ = source_rank * nbSends_;
        }
        initializedSource_ = true;
    }
    if(!initializedRecep_ && role == DECAF_REDIST_DEST )
    {
        if(nbSources_ > nbDests_ && nbSources_ % nbDests_ != 0)
        {
            std::cerr<<"ERROR : the number of destination does not divide the number of sources. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        else if(nbSources_ < nbDests_ && nbDests_ % nbSources_ != 0)
        {
            std::cerr<<"ERROR : the number of sources does not divide the number of destinations. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        if(nbSources_ > nbDests_)
            nbReceptions_ = nbSources_ / nbDests_;
        else
            nbReceptions_ = 1;
        initializedRecep_ = true;
    }

}

void
decaf::
RedistProcCCI::splitData(pConstructData& data, RedistRole role)
{
    // We don't have to do anything here
    data->serialize();
}

// point to point redistribution protocol
void
decaf::
RedistProcCCI::redistribute(pConstructData& data, RedistRole role)
{
    int ret;

    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {

        // First we send the request for the memory handler
        // and register the local memory
        for (unsigned int i = 0; i < nbSends_; i++)
        {
            if (rank_ == local_dest_rank_ + destination_ + i)
            {
                transit = data;
            }
            else if(data.getPtr()) // Full message to send
            {
                // Sending the memory request
                msg_cci req_mem;
                req_mem.type = MSG_MEM_REQ;
                req_mem.it = send_it;
                req_mem.value = data->getOutSerialBufferSize();

                int connection_index = destination_ + i;
                cci_send(channels_prod_[connection_index].connection, &req_mem, sizeof(req_mem), (void*)1, 0);

                // Deregistering the previous memory if any
                if(channels_prod_[connection_index].buffer_size > 0)
                    cci_rma_deregister(endpoint_prod_, channels_prod_[connection_index].local_rma_handle);

                // Registering the local memory
                int flags = 0; // The local memory won't be read or written by other endpoints
                ret = cci_rma_register(endpoint_prod_, data->getOutSerialBuffer(),
                                       data->getOutSerialBufferSize(), flags,
                                       &channels_prod_[connection_index].local_rma_handle);
                channels_prod_[connection_index].buffer_size = data->getOutSerialBufferSize();
            }
            else // Empty message still sent to unlock the server
            {
                msg_cci req_mem;
                req_mem.type = MSG_MEM_REQ;
                req_mem.it = send_it;
                req_mem.value = 1;

                int connection_index = destination_ + i;
                cci_send(channels_prod_[connection_index].connection, &req_mem, sizeof(req_mem), (void*)1, 0);

                // Deregistering the previous memory if any
                if(channels_prod_[connection_index].buffer_size > 0)
                    cci_rma_deregister(endpoint_prod_, channels_prod_[connection_index].local_rma_handle);

                int flags = 0;
                // We don't use the serial buffer from the pConstructData because the pointer is not initialized
                // Instead we use the buffer from the channel
                channels_prod_[connection_index].buffer.resize(1);
                ret = cci_rma_register(endpoint_prod_, &channels_prod_[connection_index].buffer[0],
                                       1, flags,
                                       &channels_prod_[connection_index].local_rma_handle);
                channels_prod_[connection_index].buffer_size = 1;
            }
        }

        int nb_send = nbSends_;

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

    // check if we have something in transit to/from self
    if (role == DECAF_REDIST_DEST && !transit.empty())
    {
        if (mergeMethod_ == DECAF_REDIST_MERGE_STEP)
            data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
        else if (mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->unserializeAndStore(transit->getOutSerialBuffer(),
                                      transit->getOutSerialBufferSize());
        transit.reset();              // we don't need it anymore, clean for the next iteration
        if (nbSources_ > nbDests_)
            nbReceptions_ = nbSources_ / nbDests_ - 1;
        else
            nbReceptions_ = 0;        // Broadcast case : we got the only data we will receive
    }

    if(role == DECAF_REDIST_DEST)
    {
        int nbRecep = nbReceptions_;
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
                    cci_send(event->recv.connection, &msg_ack, sizeof(msg_cci), (void*)1, 0);
                    fprintf(stderr, "WARINING: Reception of a RMA sent which worked. Should NOT have worked.\n");
                }
                else
                {
                    // BUG: that corresponds to the message sent from the client
                    // at the end of the RDMA operation (MSG_RMA_SENT) but the content of the
                    // msg is corrupted.

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

        if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->mergeStoredData();

        get_it++;
    }

    // Make sure that we process all the requests of 1 iteration before receiving events from the next one
    // TODO: use the queue event to manage stored events belonging to other iterations
    // NOTE Matthieu: Implemented poll_event. Have to check on a cluster to see if it holds.
    //MPI_Barrier(task_communicator_);
}
