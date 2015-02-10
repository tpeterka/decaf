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

// transport layer specific types
#ifdef TRANSPORT_MPI
#include "transport/mpi/types.hpp"
#include "transport/mpi/comm.hpp"
#include "transport/mpi/data.hpp"
#endif

#include "types.hpp"
#include "data.hpp"

namespace decaf
{
  // world is partitioned into {producer, dataflow, consumer, other} in increasing rank
  class Decaf
  {
  public:
    Decaf(CommHandle world_comm,
          DecafSizes& decaf_sizes,
          int (*selector)(Decaf*),
          void (*pipeliner)(Decaf*),
          void (*checker)(Decaf*),
          Data* data);
    ~Decaf();
    void put(void* d);
    void* get(bool no_copy = false);
    int get_nitems(bool no_copy = false)
      { return(no_copy? data_->put_nitems() : data_->get_nitems(DECAF_CON)); }
    Data* data()           { return data_; }
    DecafSizes* sizes()    { return &sizes_; }
    void flush();
    void err()             { ::all_err(err_); }
    // whether this rank is producer, dataflow, or consumer (may have multiple roles)
    bool is_prod()         { return((type_ & DECAF_PRODUCER_COMM) == DECAF_PRODUCER_COMM); }
    bool is_dflow()        { return((type_ & DECAF_DATAFLOW_COMM) == DECAF_DATAFLOW_COMM); }
    bool is_con()          { return((type_ & DECAF_CONSUMER_COMM) == DECAF_CONSUMER_COMM); }
    CommHandle prod_comm() { return prod_comm_->handle(); }
    CommHandle con_comm()  { return con_comm_->handle();  }

  private:
    CommHandle world_comm_;     // handle to original world communicator
    Comm* prod_comm_;           // producer communicator
    Comm* con_comm_;            // consumer communicator
    Comm* prod_dflow_comm_;     // communicator covering producer and dataflow
    Comm* dflow_con_comm_;      // communicator covering dataflow and consumer
    Data* data_;                // data model
    DecafSizes sizes_;          // sizes of communicators, time steps
    int  (*selector_)(Decaf*);  // user-defined selector code
    void (*pipeliner_)(Decaf*); // user-defined pipeliner code
    void (*checker_)(Decaf*);   // user-defined resilience code
    int err_;                   // last error
    CommType type_;             // whether this instance is producer, consumer, dataflow, or other
    void dataflow();
    void forward();
  };

} // namespace

decaf::
Decaf::Decaf(CommHandle world_comm,
             DecafSizes& decaf_sizes,
             int (*selector)(Decaf*),
             void (*pipeliner)(Decaf*),
             void (*checker)(Decaf*),
             Data* data):
  world_comm_(world_comm),
  prod_dflow_comm_(NULL),
  dflow_con_comm_(NULL),
  selector_(selector),
  pipeliner_(pipeliner),
  checker_(checker),
  data_(data),
  type_(DECAF_OTHER_COMM)
{
  // sizes is a POD struct, initialization not allowed in C++03; need to use assignment workaround
  const static DecafSizes sizes = {
    decaf_sizes.prod_size,  decaf_sizes.dflow_size,  decaf_sizes.con_size,
    decaf_sizes.prod_start, decaf_sizes.dflow_start, decaf_sizes.con_start,
    decaf_sizes.con_nsteps
  };
  sizes_ = sizes;

  int world_rank = CommRank(world_comm); // my place in the world
  int world_size = CommSize(world_comm);

  // ensure sizes and starts fit in the world
  if (sizes_.prod_start + sizes_.prod_size > world_size   ||
      sizes_.dflow_start + sizes_.dflow_size > world_size ||
      sizes_.con_start + sizes_.con_size > world_size)
  {
    err_ = DECAF_COMM_SIZES_ERR;
    return;
  }

  // communicators
  int prod_dflow_start = sizes_.prod_start;
  int prod_dflow_end   = std::max(sizes_.dflow_start + sizes_.dflow_size - 1,
                                  sizes_.prod_start + sizes_.prod_size - 1);
  int dflow_con_start  = std::min(sizes_.dflow_start, sizes_.con_start);
  int dflow_con_end    = sizes_.con_start + sizes_.con_size - 1;

  if (world_rank >= sizes_.prod_start &&                   // producer
      world_rank < sizes_.prod_start + sizes_.prod_size)
  {
    type_ |= DECAF_PRODUCER_COMM;
    prod_comm_ = new Comm(world_comm, sizes_.prod_start, sizes_.prod_start + sizes_.prod_size - 1);
  }
  if (world_rank >= sizes_.dflow_start &&                  // dataflow
      world_rank < sizes_.dflow_start + sizes_.dflow_size)
    type_ |= DECAF_DATAFLOW_COMM;
  if (world_rank >= sizes_.con_start &&                    // consumer
      world_rank < sizes_.con_start + sizes_.con_size)
  {
    type_ |= DECAF_CONSUMER_COMM;
    con_comm_ = new Comm(world_comm, sizes_.con_start, sizes_.con_start + sizes_.con_size - 1);
  }

  if (world_rank >= prod_dflow_start && world_rank <= prod_dflow_end)
    prod_dflow_comm_ = new Comm(world_comm, prod_dflow_start, prod_dflow_end, sizes_.prod_size,
                                sizes_.dflow_size, sizes_.dflow_start - sizes_.prod_start,
                                DECAF_PROD_DFLOW_COMM);
  if (world_rank >= dflow_con_start && world_rank <= dflow_con_end)
    dflow_con_comm_ = new Comm(world_comm, dflow_con_start, dflow_con_end, sizes_.dflow_size,
                               sizes_.con_size, sizes_.con_start - sizes_.dflow_start,
                               DECAF_DFLOW_CON_COMM);

  // dataflow ranks that overlap producer ranks run the dataflow inside of put
  // those dataflow ranks that are disjoint run the dataflow below
  if (is_dflow() && !is_prod())
    dataflow();

  err_ = DECAF_OK;
}

decaf::
Decaf::~Decaf()
{
  if (prod_dflow_comm_)
    delete prod_dflow_comm_;
  if (dflow_con_comm_)
    delete dflow_con_comm_;
  if (is_prod())
    delete prod_comm_;
  if (is_con())
    delete con_comm_;
}

void
decaf::
Decaf::put(void* d)
{
  data_->put_items(d);

  // TODO: for now executing only user-supplied custom code, need to develop primitives
  // for automatic decaf code

  for (int i = 0; i < prod_dflow_comm_->num_outputs(); i++)
  {
    // selection always needs to always be user-defined and is mandatory
    // TODO: write automatic aggregator, for now not changing the number of items selected
    data_->put_nitems(selector_(this));

    // pipelining should be automatic if the user defines the pipeline chunk unit
    if (pipeliner_)
      pipeliner_(this);

    // TODO: not looping over pipeliner chunks yet
    if (data_->put_nitems())
    {
//       fprintf(stderr, "putting to prod_dflow rank %d put_nitems = %d\n",
//               prod_dflow_comm_->start_output() + i, data_->put_nitems());
      prod_dflow_comm_->put(data_, prod_dflow_comm_->start_output() + i, DECAF_PROD);
    }
  }

  // this rank may also serve as dataflow in case producer and dataflow overlap
  if (is_dflow())
    forward();
}

void*
decaf::
Decaf::get(bool no_copy)
{
  if (no_copy)
    return data_->put_items();
  else
  {
    dflow_con_comm_->get(data_, DECAF_CON);
    return data_->get_items(DECAF_CON);
  }
}

// run the dataflow
void
decaf::
Decaf::dataflow()
{
  // TODO: when pipelining, would not store all items in dataflow before forwarding to consumer
  // as is done below
  for (int i = 0; i < sizes_.con_nsteps; i++)
    forward();
}

// forward the data through the dataflow
void
decaf::
Decaf::forward()
{
  // get from producer
  prod_dflow_comm_->get(data_, DECAF_DFLOW);

  // put to all consumer ranks
  for (int k = 0; k < dflow_con_comm_->num_outputs(); k++)
  {
    // TODO: write automatic aggregator, for now not changing the number of items from get
    data_->put_nitems(data_->get_nitems(DECAF_DFLOW));

    // pipelining should be automatic if the user defines the pipeline chunk unit
    if (pipeliner_)
      pipeliner_(this);

    // TODO: not looping over pipeliner chunks yet
    if (data_->put_nitems())
    {
//       fprintf(stderr, "putting to dflow_con rank %d put_nitems = %d\n",
//               dflow_con_comm_->start_output() + k, data_->put_nitems());
      dflow_con_comm_->put(data_, dflow_con_comm_->start_output() + k, DECAF_DFLOW);
    }
  }
}

// cleanup after each time step
void
decaf::
Decaf::flush()
{
  if (is_prod())
    prod_dflow_comm_->flush();
  if (is_dflow())
    dflow_con_comm_->flush();
  if (is_dflow() || is_con())
    data_->flush();
}

#endif
