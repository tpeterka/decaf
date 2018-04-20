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

#ifndef DECAF_REDIST_MPI_HPP
#define DECAF_REDIST_MPI_HPP

#include <iostream>
#include <assert.h>

#include <bredala/transport/redist_comp.h>
#include <bredala/transport/mpi/types.h>

namespace decaf
{


    class RedistMPI : public RedistComp
    {
    public:

    RedistMPI() :
        communicator_(MPI_COMM_NULL),
            commSources_(MPI_COMM_NULL),
            commDests_(MPI_COMM_NULL) {}
        RedistMPI(int rankSource,
                  int nbSources,
                  int rankDest,
                  int nbDests,
                  CommHandle communicator,
                  RedistCommMethod commMethod,
                  MergeMethod mergeMethod);
        virtual ~RedistMPI();

	//! makes sure that all communication requests are completed
        virtual void flush();

	//! deprecated
        void shutdown();

        virtual void clearBuffers();

    protected:

        // Compute the values necessary to determine how the data should be
        // splitted and redistributed.
        virtual void computeGlobal(pConstructData& data, RedistRole role) = 0;

        // Seperate the Data into chunks for each destination involve in the
        // component and fill the splitChunks vector
        virtual void splitData(pConstructData& data, RedistRole role) = 0;

        // Seperate system only data model. The data won't be split but duplicated
        void splitSystemData(pConstructData& data, RedistRole role);

        // Transfer the chunks from the sources to the destination. The data should be
        // be stored in the vector receivedChunks
        virtual void redistribute(pConstructData& data, RedistRole role);

        //Redistribution method using only point to point All to All communication
	//! redistribution via point to point communication
        void redistributeP2P(pConstructData& data, RedistRole role);

        //Redistribution method collective and sending only the necessary messages
	//! redistribution via collective method
        void redistributeCollective(pConstructData& data, RedistRole role);

        CommHandle communicator_;          ///< communicator for all the processes involved in redistribution
        CommHandle commSources_;           ///< communicator of the sources
        CommHandle commDests_;             ///< communicator of the destinations
        std::vector<CommRequest> reqs;     ///< pending communication requests (async. send requests from sources)
        pConstructData transit; 	   ///< used when a source and destination are overlapping (time-partitioning mode)
        int *sum_;                         ///< The aggregated array on the producer root which gathers all the ranks of the destinations for all sources
        int *destBuffer_;		   ///< Copy of the aggregated destination list (sum). Consumer root scatters this list among all destinations so that they will know how many messages they will receive.

        std::vector< pConstructData > splitBuffer_;	// Buffer of container to avoid reallocation// used by the consumer
    };

} // namespace

#endif
