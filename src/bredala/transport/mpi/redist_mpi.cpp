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

#include <bredala/transport/mpi/redist_mpi.h>

// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistMPI::RedistMPI(int rankSource,
                     int nbSources,
                     int rankDest,
                     int nbDests,
                     CommHandle world_comm, RedistCommMethod commMethod, MergeMethod mergeMethod) :
    RedistComp(rankSource, nbSources, rankDest, nbDests, commMethod, mergeMethod),
    communicator_(MPI_COMM_NULL),
    commSources_(MPI_COMM_NULL),
    commDests_(MPI_COMM_NULL),
    sum_(NULL),
    destBuffer_(NULL)
{
    MPI_Group group, groupRedist, groupSource, groupDest;
    MPI_Comm_group(world_comm, &group);

    // group covering both the sources and destinations
    // destination ranks need not be higher than source ranks

    // Testing if sources and receivers are disjoints
    if(rankDest >= rankSource + nbSources || rankSource >= rankDest + nbDests)
    {
        int range_both[2][3];
        if(rankDest < rankSource) //Separation to preserve the order of the ranks
        {
            range_both[0][0] = rankDest;
            range_both[0][1] = rankDest + nbDests - 1;
            range_both[0][2] = 1;
            range_both[1][0] = rankSource;
            range_both[1][1] = rankSource + nbSources - 1;
            range_both[1][2] = 1;
            local_source_rank_ = nbDests;
            local_dest_rank_ = 0;
        }
        else
        {
            range_both[0][0] = rankSource;
            range_both[0][1] = rankSource + nbSources - 1;
            range_both[0][2] = 1;
            range_both[1][0] = rankDest;
            range_both[1][1] = rankDest + nbDests - 1;
            range_both[1][2] = 1;
            local_source_rank_ = 0;
            local_dest_rank_ = nbSources;

        }
        MPI_Group_range_incl(group, 2, range_both, &groupRedist);

    }
    else //Sources and Receivers are overlapping
    {
        int range[3];
        range[0] = std::min(rankSource, rankDest);
        range[1] = std::max(rankSource + nbSources - 1, rankDest + nbDests - 1);
        range[2] = 1;
        MPI_Group_range_incl(group, 1, &range, &groupRedist);

        if(rankDest < rankSource) //Separation to preserve the order of the ranks
        {
            local_source_rank_ = rankSource - rankDest;
            local_dest_rank_ = 0;
        }
        else
        {
            local_source_rank_ = 0;
            local_dest_rank_  = rankDest - rankSource;
        }
    }

    MPI_Comm_create_group(world_comm, groupRedist, 0, &communicator_);
    MPI_Group_free(&groupRedist);

    MPI_Comm_rank(communicator_, &rank_);
    MPI_Comm_size(communicator_, &size_);

    // group with all the sources
    if(isSource())
    {
        int range_src[3];
        range_src[0] = rankSource;
        range_src[1] = rankSource + nbSources - 1;
        range_src[2] = 1;
        MPI_Group_range_incl(group, 1, &range_src, &groupSource);
        MPI_Comm_create_group(world_comm, groupSource, 0, &commSources_);
        MPI_Group_free(&groupSource);
        int source_rank;
        MPI_Comm_rank(commSources_, &source_rank);


        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        summerizeDest_ = new int[ nbDests_];


    }

    // group with all the destinations
    if(isDest())
    {
        int range_dest[3];
        range_dest[0] = rankDest;
        range_dest[1] = rankDest + nbDests - 1;
        range_dest[2] = 1;
        MPI_Group_range_incl(group, 1, &range_dest, &groupDest);
        MPI_Comm_create_group(world_comm, groupDest, 0, &commDests_);
        MPI_Group_free(&groupDest);
        int dest_rank;
        MPI_Comm_rank(commDests_, &dest_rank);
    }

    MPI_Group_free(&group);

    destBuffer_ = new int[nbDests_];
    sum_ = new int[nbDests_];

    transit = pConstructData(false);
}

decaf::
RedistMPI::~RedistMPI()
{
    if (communicator_ != MPI_COMM_NULL)
        MPI_Comm_free(&communicator_);
    if (commSources_ != MPI_COMM_NULL)
        MPI_Comm_free(&commSources_);
    if (commDests_ != MPI_COMM_NULL)
        MPI_Comm_free(&commDests_);

    delete[] destBuffer_;
    delete[] sum_;
}

void
decaf::
RedistMPI::splitSystemData(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete [] summerizeDest_;
        summerizeDest_ = new int[ nbDests_];
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
RedistMPI::redistribute(pConstructData& data, RedistRole role)
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
RedistMPI::redistributeCollective(pConstructData& data, RedistRole role)
{
    // debug
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
       /* // producer root sends the number of messages to root of consumer
        if (role == DECAF_REDIST_SOURCE && rank_ == local_source_rank_)
            MPI_Send(sum_,  nbDests_, MPI_INT,  local_dest_rank_, MPI_METADATA_TAG,
                     communicator_);

        // consumer root receives the number of messages
        if (role == DECAF_REDIST_DEST && rank_ ==  local_dest_rank_)
            MPI_Recv(destBuffer_,  nbDests_, MPI_INT, local_source_rank_, MPI_METADATA_TAG,
                     communicator_, MPI_STATUS_IGNORE);*/
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
}

// point to point redistribution protocol
void
decaf::
RedistMPI::redistributeP2P(pConstructData& data, RedistRole role)
{
    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {
        for(unsigned int i = 0; i <  destList_.size(); i++)
        {
            //Sending to self, we simply copy the string from the out to in
            if(destList_[i] == rank_)
            {
                transit = splitChunks_[i];
            }
            else if(destList_[i] != -1)
            {
                //fprintf(stderr, "Sending a message size: %i\n", splitChunks_[i]->getOutSerialBufferSize());
                MPI_Request req;
                reqs.push_back(req);
                MPI_Isend( splitChunks_[i]->getOutSerialBuffer(),
                           splitChunks_[i]->getOutSerialBufferSize(),
                           MPI_BYTE, destList_[i], send_data_tag, communicator_, &reqs.back());
            }
            else
            {
                MPI_Request req;
                reqs.push_back(req);

                MPI_Isend( NULL,
                           0,
                           MPI_BYTE, i + local_dest_rank_, send_data_tag, communicator_, &reqs.back());

            }
        }

        send_data_tag = (send_data_tag == INT_MAX ? MPI_DATA_TAG : send_data_tag + 1);
    }

    if(role == DECAF_REDIST_DEST)
    {
        int nbRecep;
        if(isSource()) //Overlapping case
            nbRecep = nbSources_-1;
        else
            nbRecep = nbSources_;

        for(int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, recv_data_tag, communicator_, &status);
            if (status.MPI_TAG == recv_data_tag)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                if(nitems > 0)
                {
                    //Allocating the space necessary
                    data->allocate_serial_buffer(nitems);
                    MPI_Recv(data->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                             status.MPI_TAG, communicator_, &status);

                    // The dynamic type of merge is given by the user
                    // NOTE : examin if it's not more efficient to receive everything
                    // and then merge. Memory footprint more important but allows to
                    // aggregate the allocations etc
                    if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                        data->merge();
                    else if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                        //data->unserializeAndStore();
                        data->unserializeAndStore(data->getInSerialBuffer(), nitems);
                    else
                    {
                        std::cout<<"ERROR : merge method not specified. Abording."<<std::endl;
                        MPI_Abort(MPI_COMM_WORLD, 0);
                    }

                }
                else
                {
                    MPI_Recv(NULL, 0, MPI_BYTE,status.MPI_SOURCE,
                             status.MPI_TAG, communicator_, &status);
                }
            }
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

        recv_data_tag = (recv_data_tag == INT_MAX ? MPI_DATA_TAG : recv_data_tag + 1);
    }
}

void
decaf::
RedistMPI::flush()
{
    if (reqs.size())
        MPI_Waitall(reqs.size(), &reqs[0], MPI_STATUSES_IGNORE);
    reqs.clear();

    // data have been received, we can clean it now
    // Cleaning the data here because synchronous send.
    // TODO: move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

void
decaf::
RedistMPI::shutdown()
{
    for (size_t i = 0; i < reqs.size(); i++)
        MPI_Request_free(&reqs[i]);
    reqs.clear();

    // data have been received, we can clean it now
    // Cleaning the data here because synchronous send.
    // TODO: move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

void
decaf::
RedistMPI::clearBuffers()
{
    splitBuffer_.clear();
}

