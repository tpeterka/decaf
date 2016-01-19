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

      void flush();

  protected:

      // Compute the values necessary to determine how the data should be
      // splitted and redistributed.
      virtual void computeGlobal(std::shared_ptr<BaseData> data, RedistRole role) = 0;

      // Seperate the Data into chunks for each destination involve in the
      // component and fill the splitChunks vector
      virtual void splitData(std::shared_ptr<BaseData> data, RedistRole role) = 0;

      // Transfer the chunks from the sources to the destination. The data should be
      // be stored in the vector receivedChunks
      int redistribute(std::shared_ptr<BaseData> data, RedistRole role);

      CommHandle communicator_; // Communicator for all the processes involve
      CommHandle commSources_;  // Communicator of the sources
      CommHandle commDests_;    // Communicator of the destinations
      std::vector<CommRequest> reqs;       // pending communication requests

      std::shared_ptr<BaseData> transit; // Used then a source and destination are overlapping

      int *sum_;                // Used by the producer
      int *destBuffer_;         // Used by the consumer

  };

} // namespace

#endif
