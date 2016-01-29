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
    local_source_rank_ = 0;                     // rank of first source in communicator_
    local_dest_rank_   = nbSources;             // rank of first destination in communicator_

    // group covering both the sources and destinations
    // destination ranks need not be higher than source ranks
    int range_both[2][3];
    range_both[0][0] = rankSource;
    range_both[0][1] = rankSource + nbSources - 1;
    range_both[0][2] = 1;
    range_both[1][0] = rankDest;
    range_both[1][1] = rankDest + nbDests - 1;
    range_both[1][2] = 1;
    MPI_Group_range_incl(group, 2, range_both, &groupRedist);
    MPI_Comm_create_group(world_comm, groupRedist, 0, &communicator_);
    MPI_Group_free(&groupRedist);
    MPI_Comm_rank(communicator_, &rank_);
    MPI_Comm_size(communicator_, &size_);

    // group with all the sources
    if (isSource())
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
    if (isDest())
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
    sum_        = new int[nbDests_];
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

int
decaf::
RedistMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{
    int retval = 0;                          // return value
    int flag;                                // result of MPI_Iprobe

    // sum the number of emissions for each destination
    if (role == DECAF_REDIST_SOURCE)
        MPI_Reduce(summerizeDest_, sum_, nbDests_, MPI_INT, MPI_SUM,
                   local_source_rank_, commSources_);

    // overlapping source and destination
    if (rank_ == local_source_rank_ && rank_ == local_dest_rank_)
        memcpy(destBuffer_, sum_, nbDests_ * sizeof(int));
    else
    {
        // sending to the rank 0 of the destinations
        if (role == DECAF_REDIST_SOURCE && rank_ == local_source_rank_)
        {
            MPI_Request req;
            reqs.push_back(req);
            MPI_Isend(sum_,  nbDests_, MPI_INT,  local_dest_rank_, MPI_METADATA_TAG,
                      communicator_,&reqs.back());
        }


        // getting the accumulated buffer on the destination side
        if (role == DECAF_REDIST_DEST && rank_ ==  local_dest_rank_) //Root of destination
        {
            MPI_Status status;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_METADATA_TAG, communicator_, &flag, &status);
            if (flag && status.MPI_TAG == MPI_METADATA_TAG)  // normal, non-null get
            {
                retval = 1;
                MPI_Recv(destBuffer_,  nbDests_, MPI_INT, local_source_rank_, MPI_METADATA_TAG,
                         communicator_, MPI_STATUS_IGNORE);
            }
        }
    }

    // At this point, each source knows where they have to send data

    // processing the data exchange
    if (role == DECAF_REDIST_SOURCE)
    {
        for (unsigned int i = 0; i <  destList_.size(); i++)
        {
            // sending to self, we store the data from the out to in
            if (destList_.at(i) == rank_)
                transit = splitChunks_.at(i);
            else if (destList_.at(i) != -1)
            {
                // // debug
                // int rank;
                // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                // if (rank < 4)
                //     fprintf(stderr, "g0:\n");
                MPI_Request req;
                reqs.push_back(req);
                MPI_Isend(splitChunks_.at(i)->getOutSerialBuffer(),
                          splitChunks_.at(i)->getOutSerialBufferSize(),
                          MPI_BYTE, destList_.at(i), MPI_DATA_TAG, communicator_, &reqs.back());
            }
        }
    }

    // check if we have something in transit to/from self
    if (role == DECAF_REDIST_DEST && transit)
    {
        data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
        transit.reset();              // we don't need it anymore, clean for the next iteration
        return 1;
    }

    if (!retval)
        return retval;

    // the remainder of the messaging is blocking
    // once the control information is sent/received, the payload must follow; wait for it

    // scatter the sum across all the destinations
    int nbRecep;
    if (role == DECAF_REDIST_DEST)
        MPI_Scatter(destBuffer_,  1, MPI_INT, &nbRecep, 1, MPI_INT, 0, commDests_);

    // At this point, each destination knows how many message it should receive
    if (role == DECAF_REDIST_DEST)
    {
        // debug
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank < 4)
            fprintf(stderr, "g1: nbRecep %d\n", nbRecep);

        for (int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_DATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_DATA_TAG)  // normal, non-null get
            {
                if (rank < 4)
                    fprintf(stderr, "g2:\n");
                int nbytes; // number of bytes in the message
                MPI_Get_count(&status, MPI_BYTE, &nbytes);
                data->allocate_serial_buffer(nbytes); // allocate necessary space
                MPI_Recv(data->getInSerialBuffer(), nbytes, MPI_BYTE, status.MPI_SOURCE,
                         status.MPI_TAG, communicator_, &status);

                // The dynamic type of merge is given by the user
                // NOTE: examine if it's not more efficient to receive everything
                // and then merge. Memory footprint more important but allows to
                // aggregate the allocations etc
                data->merge();
            }

        }
    }

    return 1;                                // at this point, we're sure we received everything
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
