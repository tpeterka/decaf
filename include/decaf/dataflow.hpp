//---------------------------------------------------------------------------
//
// dataflow interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATAFLOW_HPP
#define DECAF_DATAFLOW_HPP

#include "types.hpp"
#include "comm.hpp"

// transport layer implementations
#ifdef TRANSPORT_MPI
#include "transport/mpi/comm.hpp"
#endif

namespace decaf
{

  // dataflow
  struct Dataflow
  {
    int err_; // last error

    Dataflow(Comm& comm) {}
    ~Dataflow() {}

    void err() { ::all_err(err_); }
  };

} // namespace

#endif
