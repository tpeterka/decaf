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
    Datatype complete_datatype_;
    Datatype chunk_datatype_;
    enum Decomposition decomp_type_;
    void* base_addr_; // base address of complete datatype
    int err_; // last error

    Data(Datatype dtype) : complete_datatype_(dtype), base_addr_(NULL) {}
    ~Data() {}

    void* data_ptr() { return base_addr_; }
    void err() { ::all_err(err_); }

  };

} // namespace

#endif
