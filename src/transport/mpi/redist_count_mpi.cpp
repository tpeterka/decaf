//---------------------------------------------------------------------------
//
// data interface
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

#include "decaf/transport/mpi/redist_count_mpi.h"

// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistCountMPI::RedistCountMPI(int rankSource,
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
RedistCountMPI::~RedistCountMPI()
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

bool
decaf::
RedistCountMPI::isSource()
{
    // fprintf(stderr, "isSource() %d: rank_ %d local_source_rank_ %d nbSources_ %d\n",
    //         rank_ >= local_source_rank_ && rank_ < local_source_rank_ + nbSources_,
    //         rank_, local_source_rank_, nbSources_);
    return rank_ >= local_source_rank_ && rank_ < local_source_rank_ + nbSources_;
}

bool
decaf::
RedistCountMPI::isDest()
{
    // fprintf(stderr, "isDest()%d: rank_ %d local_dest_rank_ %d nbDests_ %d\n",
    //         rank_ >= local_dest_rank_ && rank_ < local_dest_rank_ + nbDests_,
    //         rank_, local_dest_rank_, nbDests_);
    return rank_ >= local_dest_rank_ && rank_ < local_dest_rank_ + nbDests_;
}


void
decaf::
RedistCountMPI::computeGlobal(std::shared_ptr<BaseData> data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        int nbItems = data->getNbItems();

        if(nbSources_ == 1)
        {
            global_item_rank_ = 0;
            global_nb_items_ = nbItems;
        }
        else
        {
            //Computing the index of the local first item in the global array of data
            MPI_Scan(&nbItems, &global_item_rank_, 1, MPI_INT,
                     MPI_SUM, commSources_);
            global_item_rank_ -= nbItems;   // Process rank 0 has the item 0,
                                            // rank 1 has the item nbItems(rank 0)
                                            // and so on

            //Compute the number of items in the global array
            MPI_Allreduce(&nbItems, &global_nb_items_, 1, MPI_INT,
                          MPI_SUM, commSources_);
        }
        std::cout<<"Total number of items : "<<global_nb_items_<<std::endl;
    }
}

void
decaf::
RedistCountMPI::splitData(std::shared_ptr<BaseData> data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE){
        // We have the items global_item_rank to global_item_rank_ + data->getNbItems()
        // We have to split global_nb_items_ into nbDest in total

        //Computing the number of elements to split
        int items_per_dest = global_nb_items_ /  nbDests_;

        int rankOffset = global_nb_items_ %  nbDests_;

        //debug
        //std::cout<<"Ranks from 0 to "<<rankOffset-1<<" will receive "<< items_per_dest+1 <<
        //           " items. Ranks from "<<rankOffset<< " to "<<  nbDests_-1<<
        //           " will reiceive" <<items_per_dest<< " items"<< std::endl;

        //Computing how to split the data

        //Compute the destination rank of the first item
        int first_rank;
        if( rankOffset == 0){ //  Case where nbDest divide the total number of item
            first_rank = global_item_rank_ / items_per_dest;
        }
        else
        {
            // The first ranks have items_per_dest+1 items
            first_rank = global_item_rank_ / (items_per_dest+1);

            //If we starts
            if(first_rank >= rankOffset)
            {
                first_rank = rankOffset +
                        (global_item_rank_ - rankOffset*(items_per_dest+1)) / items_per_dest;
            }
        }

        //Compute the split vector and the destination ranks
        std::vector<int> split_ranges;
        int items_left = data->getNbItems();
        int current_rank = first_rank;

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete  summerizeDest_;
         summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        //We have nothing to do now, the necessary data are initialized
        //if(items_left == 0)
        //    return;

        while(items_left != 0)
        {
            int currentNbItems;
            //We may have to complete the rank
            if(current_rank == first_rank){
                int global_item_firstrank;
                if(first_rank < rankOffset)
                {
                    global_item_firstrank = first_rank * (items_per_dest + 1);
                    currentNbItems = std::min(items_left,
                                         global_item_firstrank + items_per_dest + 1 - global_item_rank_);
                }
                else
                {
                    global_item_firstrank = rankOffset * (items_per_dest + 1) +
                            (first_rank - rankOffset) * items_per_dest ;
                    currentNbItems = std::min(items_left,
                                     global_item_firstrank + items_per_dest - global_item_rank_);

                }


            }
            else if(current_rank < rankOffset)
                currentNbItems = std::min(items_left, items_per_dest + 1);
            else
                currentNbItems = std::min(items_left, items_per_dest);

            split_ranges.push_back(currentNbItems);

            //We won't send a message if we send to self
            if(current_rank + local_dest_rank_ != rank_)
                summerizeDest_[current_rank] = 1;
            // rankDest_ - rankSource_ is the rank of the first destination in the
            // component communicator (communicator_)
            destList_.push_back(current_rank + local_dest_rank_);
            items_left -= currentNbItems;
            current_rank++;
        }

        splitChunks_ =  data->split( split_ranges );

        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            // TODO : Check the rank for the destination.
            // Not necessary to serialize if overlapping
            if(!splitChunks_.at(i)->serialize())
                std::cout<<"ERROR : unable to serialize one object"<<std::endl;
        }

        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping
        data->purgeData();
    }
}

void
decaf::
RedistCountMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{

    // Sum the number of emission for each destination
    if(role == DECAF_REDIST_SOURCE)
    {
        MPI_Reduce( summerizeDest_, sum_,  nbDests_, MPI_INT, MPI_SUM,
                    0, commSources_); // 0 Because we are in the source comm
                   //local_source_rank_, commSources_);
    }

    //Case with overlapping
    if(rank_ == local_source_rank_ && rank_ == local_dest_rank_)
    {
        memcpy(destBuffer_, sum_, nbDests_ * sizeof(int));
    }
    else
    {
        // Sending to the rank 0 of the destinations
        if(role == DECAF_REDIST_SOURCE && rank_ == local_source_rank_)
        {
            MPI_Request req;
            reqs.push_back(req);
            MPI_Isend(sum_,  nbDests_, MPI_INT,  local_dest_rank_, MPI_METADATA_TAG, communicator_,&reqs.back());
        }


        // Getting the accumulated buffer on the destination side
        if(role == DECAF_REDIST_DEST && rank_ ==  local_dest_rank_) //Root of destination
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_METADATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_METADATA_TAG)  // normal, non-null get
            {
                MPI_Recv(destBuffer_,  nbDests_, MPI_INT, local_source_rank_, MPI_METADATA_TAG, communicator_, MPI_STATUS_IGNORE);
            }
        }
    }

    // Scattering the sum accross all the destinations
    int nbRecep;
    if(role == DECAF_REDIST_DEST)
    {
        MPI_Scatter(destBuffer_,  1, MPI_INT, &nbRecep, 1, MPI_INT, 0, commDests_);
    }

    // At this point, each source knows where they have to send data
    // and each destination knows how many message it should receive

    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {
        for(unsigned int i = 0; i <  destList_.size(); i++)
        {
            //Sending to self, we store the data from the out to in
            if(destList_.at(i) == rank_)
            {
                transit = splitChunks_.at(i);
            }
            else
            {
                MPI_Request req;
                reqs.push_back(req);
                MPI_Isend( splitChunks_.at(i)->getOutSerialBuffer(),
                          splitChunks_.at(i)->getOutSerialBufferSize(),
                          MPI_BYTE, destList_.at(i), MPI_DATA_TAG, communicator_, &reqs.back());
            }
        }
    }

    if(role == DECAF_REDIST_DEST)
    {
        for(int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_DATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_DATA_TAG)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                //Allocating the space necessary
                data->allocate_serial_buffer(nitems);

                MPI_Recv(data->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                         status.MPI_TAG, communicator_, &status);

                // The dynamic type of merge is given by the user
                // NOTE : examin if it's not more efficient to receive everything
                // and then merge. Memory footprint more important but allows to
                // aggregate the allocations etc
                data->merge();

            }

        }

        // Checking if we have something in transit from self
        if(transit)
        {
            data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());

            //We don't need it anymore. Cleaning for the next iteration
            transit.reset();
        }

    }
}
// Merge the chunks from the vector receivedChunks into one single Data.
std::shared_ptr<decaf::BaseData>
decaf::
RedistCountMPI::merge(RedistRole role)
{
    return std::shared_ptr<BaseData>();
}

void
decaf::
RedistCountMPI::flush()
{
    if(reqs.size())
        MPI_Waitall(reqs.size(), &reqs[0], MPI_STATUSES_IGNORE);
    reqs.clear();

    // Data have been received, we can clean it now
    splitChunks_.clear();
    destList_.clear();
}

