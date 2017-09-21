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

#include <bredala/transport/file/redist_round_file.h>
#include <bredala/transport/split.h>

#include <bredala/data_model/simpleconstructdata.hpp>

void
decaf::
RedistRoundFile::computeGlobal(pConstructData& data, RedistRole role)
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
                     MPI_SUM, task_communicator_);
            global_item_rank_ -= nbItems;   // Process rank 0 has the item 0,
                                            // rank 1 has the item nbItems(rank 0)
                                            // and so on
        }

    }
}

void
decaf::
RedistRoundFile::splitData(pConstructData& data, RedistRole role)
{
    split_by_round(data, role,
                   global_item_rank_,
                   splitChunks_, splitBuffer_,
                   nbDests_, local_dest_rank_, rank_, summerizeDest_,
                   destList_, commMethod_);
}