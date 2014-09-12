//---------------------------------------------------------------------------
//
// decaf top-level interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_HPP
#define DECAF_HPP

#include "transport/mpi.h"

namespace decaf
{

enum Decomposition
{
  DECAF_ROUND_ROBIN_DECOMP,
  DECAF_CONTIG_DECOMP,
  DECAF_ZCURVE_DECOMP,
  DECAF_MAX_NUM_DECOMPS,
};

  // communication mechanism (eg MPI communicator) for producer, consumer, dataflow
  struct Comm
  {
    DecafComm comm;
    int comm_size;

    Comm(DecafComm old_comm, int new_size)
      {
      }
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

}

#endif
