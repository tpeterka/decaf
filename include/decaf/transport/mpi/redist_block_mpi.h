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

#include <decaf/redist_comp.h>
#include <decaf/transport/mpi/types.h>
#include <decaf/data_model/block.hpp>


namespace decaf
{

  class RedistBlockMPI : public RedistComp
  {
  public:
      RedistBlockMPI() :
          communicator_(MPI_COMM_NULL),
          commSources_(MPI_COMM_NULL),
          commDests_(MPI_COMM_NULL) {}
      RedistBlockMPI(int rankSource, int nbSources,
                     int rankDest, int nbDests,
                     CommHandle communicator);
      ~RedistBlockMPI();

      void flush();

  protected:

      // Compute the values necessary to determine how the data should be splitted
      // and redistributed.
      virtual void computeGlobal(std::shared_ptr<BaseData> data, RedistRole role);

      // Seperate the Data into chunks for each destination involve in the component
      // and fill the splitChunks vector
      virtual void splitData(std::shared_ptr<BaseData> data, RedistRole role);

      // Transfert the chunks from the sources to the destination. The data should be
      // be stored in the vector receivedChunks
      virtual void redistribute(std::shared_ptr<BaseData> data, RedistRole role);


      // Merge the chunks from the vector receivedChunks into one single Data.
      virtual std::shared_ptr<BaseData> merge(RedistRole role);

      // Merge the chunks from the vector receivedChunks into one single data->
      //virtual BaseData* Merge();


      void splitBlock(Block<3> & base, int nbSubblock);

      void updateBlockDomainFields();

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

      // We keep these values so we can reuse them between 2 iterations
      int global_item_rank_;    // Index of the first item in the global array
      int global_nb_items_;     // Number of items in the global array

      std::shared_ptr<BaseData> transit; // Used then a source and destination are overlapping

      std::vector<Block<3> > subblocks_; // Subblocks for the destinations

      int *sum_;                // Used by the producer
      int *destBuffer_;         // Used by the consumer


  };

} // namespace

#endif
