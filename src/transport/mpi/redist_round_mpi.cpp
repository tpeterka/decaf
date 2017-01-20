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
#include <strings.h>
#include <stdio.h>
#include <string.h>

#include <decaf/transport/mpi/redist_round_mpi.h>



void
decaf::
RedistRoundMPI::computeGlobal(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        if(!data->isCountable())
        {
            std::cout<<"ERROR : Trying to redistribute with round robin "
                     <<"but the data is not fully countable. Abording."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        int nbItems = data->getNbItems();

        if(nbSources_ == 1)
        {
            global_item_rank_ = 0;
        }
        else
        {
            //Computing the index of the local first item in the global array of data
            MPI_Scan(&nbItems, &global_item_rank_, 1, MPI_INT,
                     MPI_SUM, commSources_);
            global_item_rank_ -= nbItems;   // Process rank 0 has the item 0,
                                            // rank 1 has the item nbItems(rank 0)
                                            // and so on
        }

    }
    return;
}

void
decaf::
RedistRoundMPI::splitData(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE){

        //Computing how to split the data

        //Compute the split vector and the destination ranks
        std::vector<std::vector<int> > split_ranges = std::vector<std::vector<int> >( nbDests_);
        int nbItems = data->getNbItems();

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete [] summerizeDest_;
        summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        //Distributing the data in a round robin fashion
        for(int i = 0; i < nbItems; i++)
        {
            split_ranges[(global_item_rank_ + i) % nbDests_].push_back(i);
            split_ranges[(global_item_rank_ + i) % nbDests_].push_back(1);
        }

        for(unsigned int i = 0; i < split_ranges.size(); i++)
            split_ranges[i].push_back(split_ranges[i].size() / 2);

        //Updating the informations about messages to send

        for(unsigned int i = 0; i < split_ranges.size(); i++)
        {
            if(split_ranges.at(i).size() > 0)
            {
                destList_.push_back(i + local_dest_rank_);

                //We won't send a message if we send to self
                if(i + local_dest_rank_ != rank_)
                    summerizeDest_[i] = 1;
            }
            else
                destList_.push_back(-1);

            // We don't need a special case for P2P because
            // we create already as many sub data models as
            // destinations

        }

        if(splitBuffer_.empty())
            // We prealloc with 0 to avoid allocating too much iterations
            // The first iteration will make a reasonable allocation
            data.preallocMultiple(nbDests_, 0, splitBuffer_);
        else
        {
            // No need to adjust the number of buffer, always equal to the number of destination
            for(unsigned int i = 0; i < splitBuffer_.size(); i++)
                splitBuffer_[i]->softClean();
        }

        data->split(split_ranges, splitBuffer_);

        for(unsigned int i = 0; i < splitBuffer_.size(); i++)
            splitChunks_.push_back(splitBuffer_[i]);

        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            // TODO : Check the rank for the destination.
            // Not necessary to serialize if overlapping
            if(!splitChunks_[i]->serialize())
                std::cout<<"ERROR : unable to serialize one object"<<std::endl;
        }

        // DEPRECATED
        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping

        // data->purgeData();

    }
}
