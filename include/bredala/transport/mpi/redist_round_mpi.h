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

#ifndef DECAF_REDIST_ROUND_MPI_HPP
#define DECAF_REDIST_ROUND_MPI_HPP

#include <iostream>
#include <assert.h>

#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/redist_mpi.h>

namespace decaf
{
//! Round robin redistribution component
    class RedistRoundMPI : public RedistMPI
    {
    public:

        RedistRoundMPI() :
            RedistMPI() {}
        RedistRoundMPI(int rankSource,
                       int nbSources,
                       int rankDest,
                       int nbDests,
                       CommHandle communicator,
                       RedistCommMethod commMethod  = DECAF_REDIST_COLLECTIVE,
                       MergeMethod mergeMethod = DECAF_REDIST_MERGE_STEP) :
        RedistMPI(rankSource, nbSources, rankDest, nbDests, communicator, commMethod, mergeMethod) {}
        virtual ~RedistRoundMPI() {}

    protected:

        // Compute the values necessary to determine how the data should be splitted
        // and redistributed.
        virtual void computeGlobal(pConstructData& data, RedistRole role);

        // Seperate the Data into chunks for each destination involve in the component
        // and fill the splitChunks vector
        virtual void splitData(pConstructData& data, RedistRole role);

        unsigned long long global_item_rank_;    // Index of the first item in the global array
    };

} // namespace

#endif
