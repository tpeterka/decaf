//---------------------------------------------------------------------------
//
// consumer interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_CONSUMER_HPP
#define DECAF_CONSUMER_HPP

#include "types.hpp"
#include "comm.hpp"

// transport layer implementations
#ifdef TRANSPORT_MPI
#include "transport/mpi/comm.hpp"
#endif

namespace decaf
{

  // consumer
  struct Consumer
  {
    Data data_;
    Comm* comm_;
    int err_; // last error
    void (*con_code_)(void *);    // user-defined consumer code
    void (*sel_type_)(void *);    // user-defined selector code
    void (*pipe_type_)(void *);   // user-defined pipeliner code
    void (*aggr_type_)(void *);   // user-defined aggregator code
    void (*fault_check_)(void *); // user-defined resilience code

    Consumer(Comm* comm,
             void (*con_code)(void *),
             void (*sel_type)(void *),
             void (*pipe_type)(void *),
             void (*aggr_type)(void *),
             void (*fault_check)(void *),
             Data& data):
      comm_(comm),
      con_code_(con_code),
      sel_type_(sel_type),
      pipe_type_(pipe_type),
      aggr_type_(aggr_type),
      fault_check_(fault_check),
      data_(data) {}
    ~Consumer() {}

    void* get();
    void del_data() { delete[] (unsigned char*)data_.base_addr_; }
    void exec(void* data);
    void err() { ::all_err(err_); }
  };

} // namespace

void*
Consumer::get()
{
  data_.base_addr_ = comm_->get(1, data_.complete_datatype_);
  return data_.base_addr_;
}

void
decaf::
Consumer::exec(void* data)
{
  // compute number of chunks TODO
  int num_chunks = 0;

  for (int c = 0; c < num_chunks; c++)
  {
    // break off another chunk TODO
    void* chunk = NULL;

    // call consumer code
    con_code_(chunk);
  }
}

#endif
