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

#include "decaf/transport/mpi/redist_mpi.h"

// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistMPI::RedistMPI(int rankSource,
                     int nbSources,
                     int rankDest,
                     int nbDests,
                     CommHandle world_comm) :
    RedistComp(rankSource, nbSources, rankDest, nbDests),
    communicator_(MPI_COMM_NULL),
    commSources_(MPI_COMM_NULL),
    commDests_(MPI_COMM_NULL),
    sum_(NULL),
    destBuffer_(NULL)
{
    MPI_Group group, groupRedist, groupSource, groupDest;
    MPI_Comm_group(world_comm, &group);
    local_source_rank_ = rankSource;
    local_dest_rank_ = rankDest;

    // group covering both the sources and destinations
    // destination ranks need not be higher than source ranks

    // Testing if sources and receivers are disjoints
    if(rankDest >= rankSource + nbSources || rankSource >= rankDest + nbDests)
    {
        int range_both[2][3];
        range_both[0][0] = rankSource;
        range_both[0][1] = rankSource + nbSources - 1;
        range_both[0][2] = 1;
        range_both[1][0] = rankDest;
        range_both[1][1] = rankDest + nbDests - 1;
        range_both[1][2] = 1;
        MPI_Group_range_incl(group, 2, range_both, &groupRedist);
    }
    else //Sources and Receivers are overlapping
    {
        int range[3];
        range[0] = std::min(rankSource, rankDest);
        range[1] = std::max(rankSource + nbSources - 1, rankDest + nbDests - 1);
        range[2] = 1;
        MPI_Group_range_incl(group, 1, &range, &groupRedist);
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

#if 1  // select collective or point to point protocol

// collective redistribution protocol
void
decaf::
RedistMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{
    // debug
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // producer root collects the number of messages for each source destination
    if (role == DECAF_REDIST_SOURCE)
        MPI_Reduce(summerizeDest_, sum_, nbDests_, MPI_INT, MPI_SUM,
                   0, commSources_); // 0 Because we are in the source comm
                  //local_source_rank_, commSources_);

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
    if (role == DECAF_REDIST_DEST && transit)
    {
        data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
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
            data->merge();
        }
        recv_data_tag = (recv_data_tag == INT_MAX ? MPI_DATA_TAG : recv_data_tag + 1);
    }
}

#else  // collective or point to point protocol

// point to point redistribution protocol
// TODO: not working yet
void
decaf::
RedistMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{
    if (role == DECAF_REDIST_SOURCE)         // source ranks
    {
        fprintf(stderr, "0: destList_ size = %lu\n", destList_.size());
        for (unsigned int i = 0; i < destList_.size(); i++)
        {
            // sending to self, we simply copy the string from the out to in
            if (destList_[i] == rank_)
                transit = splitChunks_[i];
            else if (destList_[i] != -1)
            {
                fprintf(stderr, "1: dest = %d\n", destList_[i]);
                MPI_Request req;
                reqs.push_back(req);
                MPI_Isend(splitChunks_[i]->getOutSerialBuffer(),
                          splitChunks_[i]->getOutSerialBufferSize(),
                          MPI_BYTE, destList_[i], MPI_DATA_TAG, communicator_, &reqs.back());
            }
            else
            {
                fprintf(stderr, "2:\n");
                MPI_Request req;
                reqs.push_back(req);
                MPI_Isend(NULL, 0, MPI_BYTE, i + local_dest_rank_, MPI_DATA_TAG,
                           communicator_, &reqs.back());
            }
        }
    }

    int nbRecep = nbSources_ - 1;
    if (role == DECAF_REDIST_DEST)           // destination ranks
    {
        for (int i = 0; i < nbRecep; i++)
        {
            fprintf(stderr, "3: nbRecep = %d\n", nbRecep);
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_DATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_DATA_TAG)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                if (nitems > 0)
                {
                    // allocate the space necessary
                    data->allocate_serial_buffer(nitems);
                    MPI_Recv(data->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                             status.MPI_TAG, communicator_, &status);

                    // The dynamic type of merge is given by the user
                    // NOTE : examine if it's not more efficient to receive everything
                    // and then merge. Memory footprint more important but allows to
                    // aggregate the allocations etc
                    data->merge();
                }
            }
        }
        if (transit)                   // check if we have something in transit
        {
            data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
            transit.reset();           // don't need it anymore; cleanup for the next iteration
        }
    }
}

#endif  // collective or point to point protocol

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
