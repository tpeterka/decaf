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

#include <bredala/transport/mpi/redist_round_mpi.h>
#include <bredala/transport/split.h>



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

        unsigned long long nbItems = data->getNbItems();

        if(nbSources_ == 1)
        {
            global_item_rank_ = 0;
        }
        else
        {
            //Computing the index of the local first item in the global array of data
            MPI_Scan(&nbItems, &global_item_rank_, 1, MPI_UNSIGNED_LONG_LONG,
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
    split_by_round(data, role,
                   global_item_rank_,
                   splitChunks_, splitBuffer_,
                   nbDests_, local_dest_rank_, rank_, summerizeDest_,
                   destList_, commMethod_);
}
