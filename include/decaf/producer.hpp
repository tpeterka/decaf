//---------------------------------------------------------------------------
//
// producer interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_PRODUCER_HPP
#define DECAF_PRODUCER_HPP

#include "types.hpp"
#include "comm.hpp"

// transport layer implementations
#ifdef TRANSPORT_MPI
#include "transport/mpi/comm.hpp"
#endif

namespace decaf
{

  // producer
  struct Producer
  {
    Data data_;
    Comm* comm_;
    int err_; // last error
    void (*prod_code_)(void *);    // user-defined producer code

    Producer(Comm* comm,
             void (*prod_code)(void *),
             Data& data) :
      comm_(comm),
      prod_code_(prod_code),
      data_(data) {}
    ~Producer() {}

    void exec(void* data) { prod_code_(data); }
    void err() { ::all_err(err_); }
  };

} // namespace

#endif
