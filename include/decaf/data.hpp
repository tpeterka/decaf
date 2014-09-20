//---------------------------------------------------------------------------
//
// data interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATA_HPP
#define DECAF_DATA_HPP

#include "types.hpp"
#include "comm.hpp"

// transport layer implementations
#ifdef TRANSPORT_MPI
#include "transport/mpi/comm.hpp"
#endif

namespace decaf
{

  // intrinsic data description
  struct Data
  {
    Datatype complete_datatype;
    Datatype chunk_datatype;
    enum Decomposition decomp_type;
    int err_; // last error

    void err() { ::all_err(err_); }

  };

} // namespace

#endif
