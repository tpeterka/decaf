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
#include "comm.hpp"
#include "data.hpp"
#include "producer.hpp"
#include "consumer.hpp"
#include "dataflow.hpp"

// transport layer implementations
#ifdef TRANSPORT_MPI
#include "transport/mpi/comm.hpp"
#endif

namespace decaf
{

  // communication mechanism (eg MPI communicator) for producer, consumer, dataflow
  // world is partitioned into {producer, dataflow, consumer, other} in increasing rank
  struct Decaf
  {
    CommHandle world_comm_; // original world communicator
    // TODO: is the individual communicator needed?
    Comm* comm_; // communicator of producer, consumer or dataflow
    CommType type_; // whether this instance is producer, consumer, dataflow, or other
    Comm* prod_dflow_comm_; // communicator covering producer and dataflow
    Comm* dflow_con_comm_; // communicator covering dataflow and consumer
    int err_; // last error

    Decaf(CommHandle world_comm, int prod_size, int con_size, int dflow_size);
    ~Decaf();

    Comm* comm() { return comm_;}
    Comm* out_comm() { return prod_dflow_comm_;}
    Comm* in_comm() { return dflow_con_comm_;}
    int type() { return type_; }
    void err() { ::all_err(err_); }
  };

} // namespace

decaf::
Decaf::Decaf(CommHandle world_comm, int prod_size, int con_size, int dflow_size) :
  world_comm_(world_comm), comm_(NULL), prod_dflow_comm_(NULL), dflow_con_comm_(NULL)
{
  int world_rank, world_size; // place in the world
  WorldOrder(world_comm, world_rank, world_size);

  if (prod_size + con_size + dflow_size > world_size)
  {
    err_ = DECAF_COMM_SIZES_ERR;
    return;
  }

  // TODO: is comm_ (the individual communicator) needed at all?

  if (world_rank < prod_size) // producer
  {
    type_ = DECAF_PRODUCER_COMM;
    comm_ = new Comm(world_comm, 0, prod_size -1);
    prod_dflow_comm_ = new Comm(world_comm, 0, prod_size + dflow_size - 1);
  }
  else if (world_rank < prod_size + dflow_size) // dataflow
  {
    type_ = DECAF_DATAFLOW_COMM;
    comm_ = new Comm(world_comm, prod_size, prod_size + dflow_size - 1);
    prod_dflow_comm_ = new Comm(world_comm, 0, prod_size + dflow_size - 1);
    dflow_con_comm_ = new Comm(world_comm, prod_size, prod_size + dflow_size + con_size - 1);
  }
  else if (world_rank < prod_size + dflow_size + con_size) // consumer
  {
    type_ = DECAF_CONSUMER_COMM;
    comm_ = new Comm(world_comm, prod_size + dflow_size, prod_size + dflow_size + con_size - 1);
    dflow_con_comm_ = new Comm(world_comm, prod_size, prod_size + dflow_size + con_size - 1);
  }
  else // other
    type_ = DECAF_WORLD_COMM;

  // debug
//   if (comm_)
//     fprintf(stderr, "type %d rank %d of size %d\n", type_, comm_->rank(), comm_->size());
//   if (prod_dflow_comm_)
//     fprintf(stderr, "type prod_dflow rank %d of size %d\n", prod_dflow_comm_->rank(), prod_dflow_comm_->size());
//   if (dflow_con_comm_)
//     fprintf(stderr, "type dflow_con rank %d of size %d\n", dflow_con_comm_->rank(), dflow_con_comm_->size());

  err_ = DECAF_OK;
}

decaf::
Decaf::~Decaf()
{
  if (comm_)
    delete comm_;
  if (prod_dflow_comm_)
    delete prod_dflow_comm_;
  if (dflow_con_comm_)
    delete dflow_con_comm_;
}

#endif
