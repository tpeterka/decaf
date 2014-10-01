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

#include "types.hpp"
#include "data.hpp"

// transport layer specific types
#ifdef TRANSPORT_MPI
#include "transport/mpi/types.hpp"
#include "transport/mpi/comm.hpp"
#endif

namespace decaf
{

  // communication mechanism (eg MPI communicator) for producer, consumer, dataflow
  // world is partitioned into {producer, dataflow, consumer, other} in increasing rank
  struct Decaf
  {
    CommHandle world_comm_; // handle to original world communicator
    CommType type_; // whether this instance is producer, consumer, dataflow, or other
    Comm* prod_dflow_comm_; // communicator covering producer and dataflow
    Comm* dflow_con_comm_; // communicator covering dataflow and consumer
    Data* data_;
    DecafSizes sizes_;
    int (*prod_selector_)(Decaf*, int); // user-defined producer selector code
    int (*dflow_selector_)(Decaf*, int); // user-defined dataflow selector code
    void (*pipeliner_)(Decaf*); // user-defined pipeliner code
    void (*checker_)(Decaf*); // user-defined resilience code
    int err_; // last error

    Decaf(CommHandle world_comm,
          int prod_size,
          int con_size,
          int dflow_size,
          int nsteps,
          int (*prod_selector)(Decaf*, int),
          int (*dflow_selector)(Decaf*, int),
          void (*pipeliner)(Decaf*),
          void (*checker)(Decaf*),
          Data* data);

    ~Decaf();

    int type() { return type_; }
    void put(void* d);
    void* get();
    Data* data() { return data_; }
    DecafSizes* sizes() { return &sizes_; }
    void del() { data_->items_.clear(); }
    void err() { ::all_err(err_); }

  private:
    void dataflow();
  };

} // namespace

decaf::
Decaf::Decaf(CommHandle world_comm,
             int prod_size,
             int con_size,
             int dflow_size,
             int nsteps,
             int (*prod_selector)(Decaf*, int),
             int (*dflow_selector)(Decaf*, int),
             void (*pipeliner)(Decaf*),
             void (*checker)(Decaf*),
             Data* data) :
  world_comm_(world_comm),
  prod_dflow_comm_(NULL),
  dflow_con_comm_(NULL),
  prod_selector_(prod_selector),
  dflow_selector_(dflow_selector),
  pipeliner_(pipeliner),
  checker_(checker),
  data_(data)
{
  // sizes is a POD struct, initialization not allowed in C++03; need to use assignment workaround
  const static DecafSizes sizes = {prod_size, con_size, dflow_size, nsteps};
  sizes_ = sizes;

  int world_rank, world_size; // place in the world
  WorldOrder(world_comm, world_rank, world_size);

  // ensure sizes match
  if (prod_size + con_size + dflow_size > world_size)
  {
    err_ = DECAF_COMM_SIZES_ERR;
    return;
  }

  // communicators
  if (world_rank < prod_size) // producer
  {
    type_ = DECAF_PRODUCER_COMM;
    prod_dflow_comm_ = new Comm(world_comm, 0, prod_size + dflow_size - 1);
  }
  else if (world_rank < prod_size + dflow_size) // dataflow
  {
    type_ = DECAF_DATAFLOW_COMM;
    prod_dflow_comm_ = new Comm(world_comm, 0, prod_size + dflow_size - 1);
    dflow_con_comm_ = new Comm(world_comm, prod_size, prod_size + dflow_size + con_size - 1);
    dataflow();
  }
  else if (world_rank < prod_size + dflow_size + con_size) // consumer
  {
    type_ = DECAF_CONSUMER_COMM;
    dflow_con_comm_ = new Comm(world_comm, prod_size, prod_size + dflow_size + con_size - 1);
  }
  else // other
    type_ = DECAF_WORLD_COMM;

  // debug
//   if (prod_dflow_comm_)
//     fprintf(stderr, "type prod_dflow rank %d of size %d\n", prod_dflow_comm_->rank(), prod_dflow_comm_->size());
//   if (dflow_con_comm_)
//     fprintf(stderr, "type dflow_con rank %d of size %d\n", dflow_con_comm_->rank(), dflow_con_comm_->size());

  err_ = DECAF_OK;
}

decaf::
Decaf::~Decaf()
{
  if (prod_dflow_comm_)
    delete prod_dflow_comm_;
  if (dflow_con_comm_)
    delete dflow_con_comm_;
}

void
decaf::
Decaf::put(void* d)
{
  data_->dp_ = d;

  // TODO: for now executing only user-supplied custom code, need to develop primitives
  // for automatic decaf code

  for (int i = 0; i < sizes_.dflow_size; i++)
  {
    // selection always needs to always be user-defined and is mandatory
    int nitems = prod_selector_(this, i);

    // pipelining should be automatic if the user defines the pipeline chunk unit
    if (pipeliner_)
      pipeliner_(this);

    // TODO: not looping over pipeliner chunks yet
    if (nitems)
      prod_dflow_comm_->put(data_->data_ptr(), nitems, sizes_.prod_size + i,
                            data_->complete_datatype_);
  }
}

void*
decaf::
Decaf::get()
{
  dflow_con_comm_->get(*data_);
  return data_->data_ptr();
}

void
decaf::
Decaf::dataflow()
{
  // forward the data, aggregating by virtue of doing a second selection

  // TODO: when pipelining, would not store all items in dataflow before forwarding to consumer
  // as is done below
  for (int i = 0; i < sizes_.nsteps; i++)
  {
    // get from all producer ranks
    for (int j = 0; j < sizes_.prod_size; j++)
      prod_dflow_comm_->get(*data_);

    // put to all consumer ranks
    for (int k = 0; k < sizes_.con_size; k++)
    {
      // selection always needs to always be user-defined and is mandatory
      int nitems = dflow_selector_(this, k);

      // pipelining should be automatic if the user defines the pipeline chunk unit
      if (pipeliner_)
        pipeliner_(this);

      // TODO: not looping over pipeliner chunks yet
      if (nitems)
        dflow_con_comm_->put(data_->data_ptr(), nitems, sizes_.dflow_size + k,
                             data_->complete_datatype_);
    }

    data_->items_.clear();
  }
}
#endif
