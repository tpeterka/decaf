//---------------------------------------------------------------------------
//
// decaf communicator interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_COMM_HPP
#define DECAF_COMM_HPP

#include "types.hpp"
#include "data.hpp"
#include <vector>

namespace decaf
{

  // generic communication mechanism for producer, consumer, dataflow
  // ranks in communicator are contiguous in the world
  struct Comm
  {
    CommHandle handle_; // communicator handle in the transport layer
    int size_; // communicator size
    int rank_; // rank in communicator
    int min_rank_; // min (world) rank of communicator
    std::vector<CommRequest> reqs; // pending communication requests
    Comm(CommHandle world_comm, int min_rank, int max_rank);
    ~Comm();

    CommHandle handle() { return handle_; }
    int size() { return size_; }
    int rank() { return rank_; }
    int world_rank(int rank) { return(rank + min_rank_); } // world rank of any rank in this comm
    int world_rank() { return(rank_ + min_rank_); } // my world rank
    void put(Data* data, int dest, bool forward);
    void get(Data* data);
    void flush();
  };

} // namespace

#endif
