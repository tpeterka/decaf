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

#ifndef DECAF_REDIST_PROC_MPI_HPP
#define DECAF_REDIST_PROC_MPI_HPP

#include <iostream>
#include <assert.h>

#include <decaf/transport/mpi/types.h>
#include <decaf/transport/mpi/redist_mpi.h>

namespace decaf
{

    class RedistProcMPI : public RedistMPI
    {
    public:

        RedistProcMPI() :
            RedistMPI() {}
        RedistProcMPI(int rankSource,
                       int nbSources,
                       int rankDest,
                       int nbDests,
                       int id,
                       CommHandle communicator,
                       RedistCommMethod commMethod  = DECAF_REDIST_COLLECTIVE,
                       MergeMethod mergeMethod = DECAF_REDIST_MERGE_STEP) :
        RedistMPI(rankSource, nbSources, rankDest, nbDests, id, communicator, commMethod, mergeMethod) {}
        virtual ~RedistProcMPI() {}

    protected:

        // Compute the values necessary to determine how the data should be splitted
        // and redistributed.
        virtual void computeGlobal(pConstructData& data, RedistRole role);

        // Seperate the Data into chunks for each destination involve in the component
        // and fill the splitChunks vector
        virtual void splitData(pConstructData& data, RedistRole role);

        // Transfer the chunks from the sources to the destination. The data should be
        // be stored in the vector receivedChunks
        virtual void redistribute(pConstructData& data, RedistRole role);

        bool initialized_;    // Index of the first item in the global array
        int destination_;
        int nbReceptions_;
    };

} // namespace

#endif
