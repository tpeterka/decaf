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

#include <decaf/transport/redist_comp.h>
#include <decaf/transport/mpi/types.h>

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
                  CommHandle communicator);
        virtual ~RedistMPI();

        virtual void flush();
        void shutdown();

    protected:

        // Compute the values necessary to determine how the data should be
        // splitted and redistributed.
        virtual void computeGlobal(std::shared_ptr<BaseData> data, RedistRole role) = 0;

        // Seperate the Data into chunks for each destination involve in the
        // component and fill the splitChunks vector
        virtual void splitData(std::shared_ptr<BaseData> data, RedistRole role) = 0;

        // Seperate system only data model. The data won't be split but duplicated
        void splitSystemData(std::shared_ptr<BaseData> data, RedistRole role);

        // Transfer the chunks from the sources to the destination. The data should be
        // be stored in the vector receivedChunks
        void redistribute(std::shared_ptr<BaseData> data, RedistRole role);

        CommHandle communicator_;          // communicator for all the processes involved in redist
        CommHandle commSources_;           // communicator of the sources
        CommHandle commDests_;             // communicator of the destinations
        std::vector<CommRequest> reqs;     // pending communication requests
        std::shared_ptr<BaseData> transit; // used when a source and destination are overlapping
        int *sum_;                         // used by the producer
        int *destBuffer_;                  // used by the consumer
    };

} // namespace

#endif
