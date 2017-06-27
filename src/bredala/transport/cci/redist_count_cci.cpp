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
#include <sstream>

#include "bredala/transport/cci/redist_count_cci.h"

#include <bredala/data_model/simpleconstructdata.hpp>

void
decaf::
RedistCountCCI::computeGlobal(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        if(!data->isCountable())
        {
            std::cout<<"ERROR : Trying to redistribute the data with respect to the number of items "
                     <<"but the data is not fully countable. Abording."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

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
                     MPI_SUM, task_communicator_);
            global_item_rank_ -= nbItems;   // Process rank 0 has the item 0,
                                            // rank 1 has the item nbItems(rank 0)
                                            // and so on

            //Compute the number of items in the global array
            MPI_Allreduce(&nbItems, &global_nb_items_, 1, MPI_INT,
                          MPI_SUM, task_communicator_);

        }

        fprintf(stderr, "Total number of items: %d\n", global_nb_items_);
    }

}

void
decaf::
RedistCountCCI::splitData(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        if( summerizeDest_) delete [] summerizeDest_;
        summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        // Clearing the send buffer from previous iteration
        splitChunks_.clear();
        destList_.clear();

        // Case where the current data model does not have data
        if(!data.getPtr() || data->getNbItems() == 0)
        {
            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data.getPtr() || !data->hasSystem())
            {
                if(commMethod_ == DECAF_REDIST_P2P)
                {
                    while(destList_.size() < nbDests_)
                    {
                        fprintf(stderr,"Adding an empty message without system before split.\n");
                        destList_.push_back(-1);
                        splitChunks_.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = 0;
                while(destList_.size() < nbDests_)
                {
                    fprintf(stderr,"Adding an empty message with system before split.\n");
                    destList_.push_back(local_dest_rank_ + dest_rank);
                    splitChunks_.push_back(pConstructData());
                    splitChunks_.back()->copySystemFields(data);
                    splitChunks_.back()->serialize();

                    if( commMethod_ == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank_ != rank_)
                            summerizeDest_[dest_rank] = 1;
                    dest_rank++;
                }
            }
        }
        else
        {


            // We have the items global_item_rank to global_item_rank_ + data->getNbItems()
            // We have to split global_nb_items_ into nbDest in total

            //Computing the number of elements to split
            int items_per_dest = global_nb_items_ /  nbDests_;

            int rankOffset = global_nb_items_ %  nbDests_;

            //Computing how to split the data

            //Compute the destination rank of the first item
            int first_rank;
            if(items_per_dest > 0) // More items than number of destination
            {
                if( rankOffset == 0) //  Case where nbDest divide the total number of item
                {
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
            }
            else
            {
                // If there are less items then destination,
                // only global_nb_items destinations will receive 1 item
                first_rank = global_item_rank_;
            }

            fprintf(stderr, "Global_item_rank: %i, first_rank: %i\n", global_item_rank_, first_rank);


            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data->hasSystem())
            {
                if(commMethod_ == DECAF_REDIST_P2P)
                {
                    while(destList_.size() < first_rank)
                    {
                        fprintf(stderr,"Adding an empty message without system before split.\n");
                        destList_.push_back(-1);
                        splitChunks_.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = 0;
                while(destList_.size() < first_rank)
                {
                    fprintf(stderr,"Adding an empty message with system before split.\n");
                    destList_.push_back(local_dest_rank_ + dest_rank);
                    splitChunks_.push_back(pConstructData());
                    splitChunks_.back()->copySystemFields(data);
                    splitChunks_.back()->serialize();

                    if( commMethod_ == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank_ != rank_)
                            summerizeDest_[dest_rank] = 1;
                    dest_rank++;
                }
            }

            fprintf(stderr, "Size of chunks before append of buffers: %lu\n", splitChunks_.size());

            //Compute the split vector and the destination ranks
            std::vector<int> split_ranges;
            int items_left = data->getNbItems();
            int current_rank = first_rank;

            unsigned int nbChunks = 0;
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
                nbChunks++;
            }

            // Shaping the buffers, ie the results of the split
            if(splitBuffer_.empty() && nbChunks > 0)
                // We prealloc with 0 to avoid allocating too much iterations
                // The first iteration will make a reasonable allocation
                data.preallocMultiple(nbChunks, 0, splitBuffer_);
            else if(nbChunks > 0)
            {
                // If we have more buffer than needed, the remove the excedent
                if(splitBuffer_.size() > nbChunks)
                    splitBuffer_.resize(nbChunks);

                for(unsigned int i = 0; i < splitBuffer_.size(); i++)
                    splitBuffer_[i]->softClean();

                // If we don't have enough buffers, we complete it
                if(nbChunks - splitBuffer_.size() > 0)
                {
                    std::vector< pConstructData > newBuffers;
                    data.preallocMultiple(nbChunks - splitBuffer_.size(), 0, newBuffers);
                    splitBuffer_.insert(splitBuffer_.end(), newBuffers.begin(), newBuffers.end());
                }

            }

            std::stringstream ss;
            ss<<"Range :[";
            for (unsigned int i = 0; i < split_ranges.size(); i++)
                ss<<split_ranges[i]<<",";
            ss<<"]";
            fprintf(stderr, "%s\n", ss.str().c_str());

            data->split(split_ranges, splitBuffer_);

            std::stringstream ss1;
            ss1<<"Generated :[";
            for (unsigned int i = 0; i < split_ranges.size(); i++)
                ss1<<splitBuffer_[i]->getNbItems()<<",";
            ss1<<"]";
            fprintf(stderr, "%s\n", ss1.str().c_str());

            for(unsigned int i = 0; i < splitBuffer_.size(); i++)
                splitChunks_.push_back(splitBuffer_[i]);

            for(unsigned int i = 0; i < splitChunks_.size(); i++)
            {
                // TODO : Check the rank for the destination.
                // Not necessary to serialize if overlapping
                if(!splitChunks_[i].empty() && !splitChunks_[i]->serialize())
                    std::cout<<"ERROR : unable to serialize one object"<<std::endl;
            }

            fprintf(stderr, "Size of chunks after append of buffers: %lu\n", splitChunks_.size());

            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data->hasSystem())
            {
                if(commMethod_ == DECAF_REDIST_P2P)
                {
                    while(destList_.size() < nbDests_)
                    {
                        fprintf(stderr,"Adding an empty message without system before split.\n");
                        destList_.push_back(-1);
                        splitChunks_.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = current_rank;
                while(destList_.size() < nbDests_)
                {
                    fprintf(stderr,"Adding an empty message with system before split.\n");
                    destList_.push_back(local_dest_rank_ + dest_rank);
                    splitChunks_.push_back(pConstructData());
                    splitChunks_.back()->copySystemFields(data);
                    splitChunks_.back()->serialize();

                    if( commMethod_ == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank_ != rank_)
                            summerizeDest_[dest_rank] = 1;
                    dest_rank++;
                }
            }

            // DEPRECATED
            // Everything is done, now we can clean the data.
            // Data might be rewriten if producers and consummers are overlapping

            // data->purgeData();

            fprintf(stderr, "Size of chunks at the end: %lu\n", splitChunks_.size());

            std::stringstream ss2;
            ss2<<"Chunks :[";
            for (unsigned int i = 0; i < splitChunks_.size(); i++)
            {
                if(splitChunks_[i].getPtr())
                    ss2<<splitChunks_[i]->getNbItems()<<",";
                else
                    ss2<<"0,";
            }
            ss2<<"]";
            fprintf(stderr, "%s\n", ss2.str().c_str());

        }
    }
}
