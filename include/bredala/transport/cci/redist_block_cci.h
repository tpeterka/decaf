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

#ifndef DECAF_REDIST_BLOCK_CCI_HPP
#define DECAF_REDIST_BLOCK_CCI_HPP

#include <iostream>
#include <assert.h>

#include <bredala/transport/mpi/types.h>
#include <bredala/transport/cci/redist_cci.h>
#include <bredala/data_model/block.hpp>
#include <bredala/data_model/constructtype.h>


namespace decaf
{

    class RedistBlockCCI : public RedistCCI
    {
    public:

        RedistBlockCCI() :
            RedistCCI() {}
        RedistBlockCCI(int rankSource,
                       int nbSources,
                       int rankDest,
                       int nbDests,
                       int global_rank,
                       CommHandle communicator,
                       std::string name,
                       RedistCommMethod commMethod  = DECAF_REDIST_COLLECTIVE,
                       MergeMethod mergeMethod = DECAF_REDIST_MERGE_STEP) :
        RedistCCI(rankSource, nbSources, rankDest, nbDests, global_rank, communicator, name, commMethod, mergeMethod) {}

        virtual ~RedistBlockCCI() {}

        //virtual void flush();

    protected:

        // Compute the values necessary to determine how the data should be splitted
        // and redistributed.
        virtual void computeGlobal(pConstructData& data, RedistRole role);

        // Seperate the Data into chunks for each destination involve in the component
        // and fill the splitChunks vector
        virtual void splitData(pConstructData& data, RedistRole role);


        //virtual void redistribute(pConstructData data, RedistRole role);

        void splitBlock(Block<3> & base, int nbSubblock);

        void updateBlockDomainFields();

        // We keep these values so we can reuse them between 2 iterations
        int global_item_rank_;    // Index of the first item in the global array
        int global_nb_items_;     // Number of items in the global array

        std::vector<Block<3> > subblocks_; // Subblocks for the destinations
    };


} // namespace

#endif
