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

#include <bredala/transport/file/redist_file.h>

using namespace std;


// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistFile::RedistFile(int rankSource,
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
    name_(name)
{

    // For File components, we only have the communicator of the local task
    // The rank is the global rank in the application =! from the MPI redist components
    rank_ = global_rank;
    local_dest_rank_ = rankDest;
    local_source_rank_ = rankSource;

    fprintf(stderr, "Global rank: %d\n", rank_);

    MPI_Comm_rank(task_communicator_, &task_rank_);
    MPI_Comm_size(task_communicator_, &task_size_);


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
RedistFile::~RedistFile()
{
    // We don't clean the communicator, it was created by the app
    //if (task_communicator_ != MPI_COMM_NULL)
    //    MPI_Comm_free(&task_communicator_);


    delete[] destBuffer_;
    delete[] sum_;
}

void
decaf::
RedistFile::splitSystemData(pConstructData& data, RedistRole role)
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
RedistFile::redistribute(pConstructData& data, RedistRole role)
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
RedistFile::redistributeCollective(pConstructData& data, RedistRole role)
{
    fprintf(stderr, "ERROR: calling COLLECTIVE File\n");
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
RedistFile::redistributeP2P(pConstructData& data, RedistRole role)
{
    fprintf(stderr, "Redistribution by file not implemented yet.");

    // Make sure that we process all the requests of 1 iteration before receiving events from the next one
    // TODO: use the queue event to manage stored events belonging to other iterations
    // NOTE Matthieu: Implemented poll_event. Have to check on a cluster to see if it holds.
    //MPI_Barrier(task_communicator_);
}

void
decaf::
RedistFile::flush()
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
RedistFile::shutdown()
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
RedistFile::clearBuffers()
{
    splitBuffer_.clear();
}

