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

#ifndef DECAF_REDIST_BLOCK_MPI_HPP
#define DECAF_REDIST_BLOCK_MPI_HPP

#include <iostream>
#include <assert.h>

#include <decaf/transport/mpi/types.h>
#include <decaf/transport/mpi/redist_mpi.h>
#include <decaf/data_model/block.hpp>
#include <decaf/data_model/constructtype.h>


namespace decaf
{

    class RedistBlockMPI : public RedistMPI
    {
    public:

        RedistBlockMPI() :
            RedistMPI() {}
        RedistBlockMPI(int rankSource,
                       int nbSources,
                       int rankDest,
                       int nbDests,
                       CommHandle communicator) :
             RedistMPI(rankSource, nbSources, rankDest, nbDests, communicator) {}
        virtual ~RedistBlockMPI() {}

    protected:

        // Compute the values necessary to determine how the data should be splitted
        // and redistributed.
        virtual void computeGlobal(std::shared_ptr<BaseData> data, RedistRole role);

        // Seperate the Data into chunks for each destination involve in the component
        // and fill the splitChunks vector
        virtual void splitData(std::shared_ptr<BaseData> data, RedistRole role);


        virtual void redistribute(std::shared_ptr<BaseData> data, RedistRole role);

        void splitBlock(Block<3> & base, int nbSubblock);

        void updateBlockDomainFields();

        virtual void flush();


        std::vector< std::shared_ptr<ConstructData> > splitBuffer_;	// Buffer of container to avoid reallocation

        // We keep these values so we can reuse them between 2 iterations
        int global_item_rank_;    // Index of the first item in the global array
        int global_nb_items_;     // Number of items in the global array

        std::vector<Block<3> > subblocks_; // Subblocks for the destinations
    };


} // namespace

#endif
