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

#ifndef DECAF_REDIST_PROC_CCI_HPP
#define DECAF_REDIST_PROC_CCI_HPP

#include <iostream>
#include <assert.h>

#include <bredala/transport/cci/redist_cci.h>

namespace decaf
{

    class RedistProcCCI : public RedistCCI
    {
    public:

        RedistProcCCI() :
            RedistCCI() {}

        RedistProcCCI(int rankSource,
                      int nbSources,
                      int rankDest,
                      int nbDests,
                      int global_rank,
                      CommHandle communicator,
                      std::string name,
                      RedistCommMethod commMethod  = DECAF_REDIST_COLLECTIVE,
                      MergeMethod mergeMethod = DECAF_REDIST_MERGE_STEP) :
       RedistCCI(rankSource, nbSources, rankDest, nbDests, global_rank, communicator, name, commMethod, mergeMethod),
                initializedSource_(false),
                initializedRecep_(false){}

        virtual ~RedistProcCCI() {}


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

        bool initializedSource_;
        bool initializedRecep_;
        int destination_;     // Rank of the first destination to send data (starting at 0)
        int nbSends_;         // Number of identicaf message to send to different destinations
        int nbReceptions_;
    };

} // namespace

#endif
