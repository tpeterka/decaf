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

#ifndef DECAF_REDIST_ZCURVE_MPI_HPP
#define DECAF_REDIST_ZCURVE_MPI_HPP

#include <iostream>
#include <assert.h>

#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/redist_mpi.h>

namespace decaf
{

    class RedistZCurveMPI : public RedistMPI
    {
    public:

        RedistZCurveMPI() :
        RedistMPI() {}
        RedistZCurveMPI(int rankSource,
                        int nbSources,
                        int rankDest,
                        int nbDests,
                        CommHandle communicator,
                        RedistCommMethod commMethod  = DECAF_REDIST_COLLECTIVE,
                        MergeMethod mergeMethod = DECAF_REDIST_MERGE_STEP,
                        std::vector<float> bBox = std::vector<float>(),
                        std::vector<int> slices = std::vector<int>());
        virtual ~RedistZCurveMPI() {}
    protected:

        // Compute the values necessary to determine how the data should be splitted
        // and redistributed.
        virtual void computeGlobal(pConstructData& data, RedistRole role);

        // Seperate the Data into chunks for each destination involve in the component
        // and fill the splitChunks vector
        virtual void splitData(pConstructData& data, RedistRole role);

        // We keep these values so we can reuse them between 2 iterations
        int global_item_rank_;    // Index of the first item in the global array
        int global_nb_items_;     // Number of items in the global array

        bool bBBox_;
        std::vector<float> bBox_;
        std::vector<int> slices_;
        std::vector<float> slicesDelta_;
        int indexes_per_dest_;
        int rankOffset_;
    };

} // namespace

#endif
