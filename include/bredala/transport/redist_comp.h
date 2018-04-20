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

#include <bredala/data_model/pconstructtype.h>
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


    /** \brief Common interface for the redistribution component (MxN).
     * Currently supported transport layers are MPI and CCI.
     */
    class RedistComp
    {
    public:
        RedistComp()
        {
            send_data_tag = MPI_DATA_TAG;
            recv_data_tag = MPI_DATA_TAG;
        }

        /// @param rankSource:       		rank 0 (root) of the sources
        /// @param nbSources:       		number of sources
        /// @param rankDest:       		rank 0 (root) of the destinations
        /// @param nbDests:       		number of destinations
        /// @param commMethod:       		strategy to use during redistribution
        /// @param mergeMethod:       		merge method to use when aggregating the received subdata models into one data model

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

	//! runs the pipeline of operations to redistribute the data (which is the only function that is called from the main program)
        void process(pConstructData& data, RedistRole role);

        // Run the pipeline of operation to receive a msg
        // False if no msg are waiting
	//! runs the pipeline of operations to receive a data. Asynchronous version is not implemented yet.
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

	//! computes the global values necessary to determine how to split and redistribute the data.
        virtual void computeGlobal(pConstructData& data, RedistRole role)=0;

	//! system data is treated separately since it needs to be sent to every destination. Hence, system data will be duplicated instead of splitting.
        virtual void splitSystemData(pConstructData& data, RedistRole role)=0;

        // Seperate the Data into chunks for each destination involve in the
        // component and fill the splitChunks vector
	//! splits the data into subdata models (chunks) for each destination
        virtual void splitData(pConstructData& data, RedistRole role)=0;

        // Transfer the chunks from the sources to the destination. The data
        // should be stored in the vector receivedChunks
	//! transfers the subdata models (chunks) from sources to the destination.
        virtual void redistribute(pConstructData& data, RedistRole role)=0;

        // Merge the chunks from the vector receivedChunks into one single Data.
	//! aggregates all the received subdata models (chunks) into one data model
        pConstructData merge(RedistRole role);

        int rankSource_;                   ///< Rank 0 (root) of the sources
        int nbSources_;                    ///< Number of sources (supposed to be consecutive?)
        int rankDest_;                     ///< Rank 0 (root) of the destinations
        int nbDests_;                      ///< Number of destinations

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
        int* summerizeDest_;            ///< An array for the source which has the ranks of all the destinations that it has to send a message. (0 is no send to that rank, 1 is send. Used only with commMethod = DECAF_REDIST_COLLECTIVE)
        std::vector<int> destList_;	///< Destination list for sources: an ID rank if a message should be sent, -1 if the message is empty. (While summerizeDest_ is used for letting consumers know about the incoming messages, destList is used for determining whom to send the message at the producer side.)

        RedistCommMethod commMethod_;        //Strategy to use during redistribution: collective or point to point.
        MergeMethod mergeMethod_;	     ///< Merge method to use when aggregating the received subdata models into one data model.
    };

} //namespace decaf

#endif
