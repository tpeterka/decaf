//---------------------------------------------------------------------------
//
// decaf communicator interface
// TODO: includes other structs for now too, factor into separate headers when they grow
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

// transport layer specific types
#include "transport/mpi/types.hpp"

namespace decaf
{

  // communication mechanism (eg MPI communicator) for producer, consumer, dataflow
  struct Comm
  {
    DecafComm comm_; // communicator
    CommType type_; // rank is producer, consumer, dataflow, or other
    int size_; // communicator size
    int rank_; // rank in communicator

    Comm(DecafComm old_comm, int prod_size, int con_size, int dflow_size, int err);
    ~Comm() { //MPI_Comm_free(&comm_);
}
    int size() { return size_; }
    int rank() { return rank_; }
    int type() { return type_; }
  };

  // intrinsic data description
  struct Data
  {
    DecafDatatype complete_datatype;
    DecafDatatype chunk_datatype;
    enum Decomposition decomp_type;
  };

  // producer
  struct Producer
  {
    Data data_;
    Comm comm_;
    void (*prod_code_)(void *);    // user-defined producer code

    Producer(Comm& comm,
             void (*prod_code)(void *),
             Data& data) :
      comm_(comm),
      prod_code_(prod_code),
      data_(data)
      {
      }

    void exec(void* data)
      {
        prod_code_(data);
      }
  };

  // consumer
  struct Consumer
  {
    Data data_;
    Comm comm_;
    void (*con_code_)(void *);    // user-defined consumer code
    void (*sel_type_)(void *);    // user-defined selector code
    void (*pipe_type_)(void *);   // user-defined pipeliner code
    void (*aggr_type_)(void *);   // user-defined aggregator code
    void (*fault_check_)(void *); // user-defined resilience code

    Consumer(Comm& comm,
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
      data_(data)
      {
      }

    void exec(void* data)
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
  };

  // dataflow
  struct Dataflow
  {
    Dataflow(Comm& comm)
      {
      }
  };

} // namespace

#endif
