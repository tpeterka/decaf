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

    Producer(Comm* comm, Data& data) :
      comm_(comm), data_(data) {}
    ~Producer() {}

    void put(void* d);
    void* data_ptr() { return data_.data_ptr(); }
    void err() { ::all_err(err_); }
  };

} // namespace

void
Producer::put(void* d)
{
  data_.base_addr_ = d;
  comm_->put(data_.base_addr_, 1, data_.complete_datatype_);
}

#endif
