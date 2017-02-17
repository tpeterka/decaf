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

#ifndef DECAF_REDIST_COMP_H
#define DECAF_REDIST_COMP_H

#include <decaf/data_model/pconstructtype.h>
#include <vector>
#include <memory>
#include <iostream>

namespace decaf
{

    enum RedistRole
    {
        DECAF_REDIST_SOURCE = 0,
        DECAF_REDIST_DEST,
    };

    enum mpiTags {
        MPI_METADATA_TAG = 1,
        MPI_DATA_TAG,
    };

    enum RedistCommMethod
    {
        DECAF_REDIST_COLLECTIVE,
        DECAF_REDIST_P2P,
    };

    enum MergeMethod
    {
        DECAF_REDIST_MERGE_ONCE,
        DECAF_REDIST_MERGE_STEP,
    };

    // This class defines the common interface for the redistribution component (MxN)
    // This interface is independant from the datatype or the transport
    // implementation. Specialized components will implement the redistribution
    // in fonction of transport layer
    class RedistComp
    {
    public:
        RedistComp()
        {
            send_data_tag = MPI_DATA_TAG;
            recv_data_tag = MPI_DATA_TAG;
        }

        RedistComp(int rankSource,
               int nbSources,
               int rankDest,
               int nbDests,
               RedistCommMethod commMethod  = DECAF_REDIST_COLLECTIVE,
               MergeMethod mergeMethod = DECAF_REDIST_MERGE_STEP) :
            rankSource_(rankSource),
            nbSources_(nbSources),
            rankDest_(rankDest),
            nbDests_(nbDests),
            summerizeDest_(NULL),
            commMethod_(commMethod),
            mergeMethod_(mergeMethod)
            {
                send_data_tag = MPI_DATA_TAG;
                recv_data_tag = MPI_DATA_TAG;
            }

        virtual ~RedistComp(){}

        // Run the pipeline of operations to redistribute the data.
        // This function is the only one that should be called from the main program
        void process(pConstructData& data, RedistRole role);

        // Run the pipeline of operation to receive a msg
        // False if no msg are waiting
        bool IGet(pConstructData& data);

        int getRankSource();
        int getNbSources();
        int getRankDest();
        int getNbDest();

        bool isSource();
        bool isDest();

        virtual void flush()    = 0;
        virtual void shutdown() = 0;

        virtual void clearBuffers() = 0;

    protected:

        // Compute the values necessary to determine how the data should be
        // splitted and redistributed.
        virtual void computeGlobal(pConstructData& data, RedistRole role)=0;

        // Seperate system only data model. The data won't be split but duplicated
        virtual void splitSystemData(pConstructData& data, RedistRole role)=0;

        // Seperate the Data into chunks for each destination involve in the
        // component and fill the splitChunks vector
        virtual void splitData(pConstructData& data, RedistRole role)=0;

        // Transfer the chunks from the sources to the destination. The data
        // should be stored in the vector receivedChunks
        virtual void redistribute(pConstructData& data, RedistRole role)=0;

        // Merge the chunks from the vector receivedChunks into one single Data.
        pConstructData merge(RedistRole role);

        int rankSource_;                   // Rank of the first source (=sender)
        int nbSources_;                    // Number of sources, supposed to be consecutive
        int rankDest_;                     // Rank of the first destination (=receiver)
        int nbDests_;                      // Number of destinations

        // TODO: check for redundancy from above members
        int rank_;                         // Rank in the group communicator
        int size_;                         // Size of the group communicator
        int local_source_rank_;            // Rank of the first source in communicator_
        int local_dest_rank_;              // Rank of the first destination in communicator_

        // send and recv data tags keep messages together from different ranks but otherwise in the
        // same workflow link and sent in the same iteration
        // data tags get incremented on each iteration until reaching INT_MAX; then reset
        int send_data_tag;
        int recv_data_tag;

        std::vector<pConstructData > splitChunks_;
        //std::vector<std::shared_ptr<char> > receivedChunks_;
        int* summerizeDest_;
        std::vector<int> destList_;

        RedistCommMethod commMethod_;        //Strategy to use during redistribute()
        MergeMethod mergeMethod_;
    };

} //namespace decaf

#endif
