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

namespace decaf
{

  // generic communication mechanism for producer, consumer, dataflow
  struct Comm
  {
    CommHandle handle_; // communicator handle in the transport layer
    int size_; // communicator size
    int rank_; // rank in communicator

    Comm(CommHandle world_comm, CommType type, int world_rank);
    Comm(CommHandle world_comm, int min_rank, int max_rank);
    ~Comm();

    CommHandle handle() { return handle_; }
    int size() { return size_; }
    int rank() { return rank_; }
    void put(void* addr, int num, int dest, Datatype dtype);
    void get(Data& data);

  };

} // namespace

#endif
