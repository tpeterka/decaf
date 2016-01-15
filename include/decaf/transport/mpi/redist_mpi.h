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

#include <decaf/redist_comp.h>
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
      ~RedistMPI();

      void flush();

  protected:

      // Transfer the chunks from the sources to the destination. The data should be
      // be stored in the vector receivedChunks
      virtual int redistribute(std::shared_ptr<BaseData> data, RedistRole role);

      // Merge the chunks from the vector receivedChunks into one single Data.
      virtual std::shared_ptr<BaseData> merge(RedistRole role);

      bool isSource();
      bool isDest();

      CommHandle communicator_; // Communicator for all the processes involve
      CommHandle commSources_;  // Communicator of the sources
      CommHandle commDests_;    // Communicator of the destinations
      std::vector<CommRequest> reqs;       // pending communication requests

      int rank_;                // Rank in the group communicator
      int size_;                // Size of the group communicator
      int local_source_rank_;   // Rank of the first source in communicator_
      int local_dest_rank_;     // Rank of the first destination in communicator_

      std::shared_ptr<BaseData> transit; // Used then a source and destination are overlapping

      int *sum_;                // Used by the producer
      int *destBuffer_;         // Used by the consumer

  };

} // namespace

#endif
