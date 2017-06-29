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

#include <bredala/transport/mpi/redist_count_mpi.h>
#include <bredala/transport/split.h>

#include <bredala/data_model/simpleconstructdata.hpp>

void
decaf::
RedistCountMPI::computeGlobal(pConstructData& data, RedistRole role)
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
                     MPI_SUM, commSources_);
            global_item_rank_ -= nbItems;   // Process rank 0 has the item 0,
                                            // rank 1 has the item nbItems(rank 0)
                                            // and so on

            //Compute the number of items in the global array
            MPI_Allreduce(&nbItems, &global_nb_items_, 1, MPI_INT,
                          MPI_SUM, commSources_);

        }
    }

}

void
decaf::
RedistCountMPI::splitData(pConstructData& data, RedistRole role)
{
/*    if(role == DECAF_REDIST_SOURCE)
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

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        if( summerizeDest_) delete [] summerizeDest_;
        summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything


        // For P2P, destList must have the same size as the number of destination.
        // We fill the destinations with no messages with -1 (= send empty message)
        if(!data->hasSystem())
        {
            if(commMethod_ == DECAF_REDIST_P2P)
            {
                while(destList_.size() < first_rank)
                {
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


        data->split(split_ranges, splitBuffer_);

        for(unsigned int i = 0; i < splitBuffer_.size(); i++)
            splitChunks_.push_back(splitBuffer_[i]);

        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            // TODO : Check the rank for the destination.
            // Not necessary to serialize if overlapping
            if(!splitChunks_[i].empty() && !splitChunks_[i]->serialize())
                std::cout<<"ERROR : unable to serialize one object"<<std::endl;
        }

        // For P2P, destList must have the same size as the number of destination.
        // We fill the destinations with no messages with -1 (= send empty message)
        if(!data->hasSystem())
        {
            if(commMethod_ == DECAF_REDIST_P2P)
            {
                while(destList_.size() < nbDests_)
                {
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

    }*/

    if(role == DECAF_REDIST_SOURCE)
    {
        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        if( !summerizeDest_)
            summerizeDest_ = new int[ nbDests_];

    }

    split_by_count(data, role,
                   global_nb_items_, global_item_rank_,
                   splitChunks_, splitBuffer_,
                   nbDests_, local_dest_rank_, rank_, summerizeDest_,
                   destList_, commMethod_);
}
