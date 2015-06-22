//---------------------------------------------------------------------------
// dataflow interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATAFLOW_HPP
#define DECAF_DATAFLOW_HPP

#include <map>

// transport layer specific types
#ifdef TRANSPORT_MPI
#include "transport/mpi/types.hpp"
#include "transport/mpi/comm.hpp"
#include "transport/mpi/data.hpp"
//New redistribution component
#include "transport/mpi/redist_count_mpi.hpp"
#include "transport/mpi/redist_round_mpi.hpp"
#include "transport/mpi/redist_zcurve_mpi.hpp"
#include <memory>
#endif

#include "types.hpp"
#include "data.hpp"

namespace decaf
{
  // world is partitioned into {producer, dataflow, consumer, other} in increasing rank
  class Dataflow
  {
  public:
    Dataflow(CommHandle world_comm,
             DecafSizes& decaf_sizes,
             void (*pipeliner)(Dataflow*),
             void (*checker)(Dataflow*),
             Data* data,
             Decomposition prod_dflow_redist = DECAF_CONTIG_DECOMP,
             Decomposition dflow_cons_redist = DECAF_CONTIG_DECOMP);
    ~Dataflow();
    void run();
    void put(void* d, int count = 1);
    void* get(bool no_copy = false);
    void put(std::shared_ptr<BaseData> data, TaskType role);
    void get(std::shared_ptr<BaseData> data, TaskType role);
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
    CommHandle prod_comm_handle() { return prod_comm_->handle(); }
    CommHandle con_comm_handle()  { return con_comm_->handle();  }
    Comm* prod_comm()             { return prod_comm_; }
    Comm* con_comm()              { return con_comm_;  }
    void forward();             //NOTE : was private

  private:
    CommHandle world_comm_;     // handle to original world communicator
    Comm* prod_comm_;           // producer communicator
    Comm* con_comm_;            // consumer communicator
    Comm* prod_dflow_comm_;     // communicator covering producer and dataflow
    Comm* dflow_con_comm_;      // communicator covering dataflow and consumer
    RedistComp* redist_prod_dflow_;  // Redistribution component between producer and dataflow
    RedistComp* redist_dflow_con_;   // Redestribution component between a dataflow and consummer
    Data* data_;                // data model
    DecafSizes sizes_;          // sizes of communicators, time steps
    void (*pipeliner_)(Dataflow*); // user-defined pipeliner code
    void (*checker_)(Dataflow*);   // user-defined resilience code
    int err_;                   // last error
    CommType type_;             // whether this instance is producer, consumer, dataflow, or other
    void dataflow();

  };

} // namespace

decaf::
Dataflow::Dataflow(CommHandle world_comm,
                   DecafSizes& decaf_sizes,
                   void (*pipeliner)(Dataflow*),
                   void (*checker)(Dataflow*),
                   Data* data,
                   Decomposition prod_dflow_redist,
                   Decomposition dflow_cons_redist):
  world_comm_(world_comm),
  prod_dflow_comm_(NULL),
  dflow_con_comm_(NULL),
  redist_prod_dflow_(NULL),
  redist_dflow_con_(NULL),
  pipeliner_(pipeliner),
  checker_(checker),
  data_(data),
  type_(DECAF_OTHER_COMM)
{
  // sizes is a POD struct, initialization not allowed in C++03; need to use assignment workaround
  // TODO: time for C++11?
  sizes_.prod_size = decaf_sizes.prod_size;
  sizes_.dflow_size = decaf_sizes.dflow_size;
  sizes_.con_size = decaf_sizes.con_size;
  sizes_.prod_start = decaf_sizes.prod_start;
  sizes_.dflow_start = decaf_sizes.dflow_start;
  sizes_.con_start = decaf_sizes.con_start;
  sizes_.con_nsteps = decaf_sizes.con_nsteps;

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
  {
    prod_dflow_comm_ = new Comm(world_comm, prod_dflow_start, prod_dflow_end, sizes_.prod_size,
                                sizes_.dflow_size, sizes_.dflow_start - sizes_.prod_start,
                                DECAF_PROD_DFLOW_COMM);
    switch(prod_dflow_redist)
    {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            std::cout<<"Using Round for prod -> dflow"<<std::endl;
            redist_prod_dflow_ = new RedistRoundMPI(sizes_.prod_start, sizes_.prod_size,
                                               sizes_.dflow_start, sizes_.dflow_size,
                                               world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            std::cout<<"Using Count for prod -> dflow"<<std::endl;
            redist_prod_dflow_ = new RedistCountMPI(sizes_.prod_start, sizes_.prod_size,
                                               sizes_.dflow_start, sizes_.dflow_size,
                                               world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            std::cout<<"Using ZCurve for prod -> dflow"<<std::endl;
            redist_prod_dflow_ = new RedistZCurveMPI(sizes_.prod_start, sizes_.prod_size,
                                               sizes_.dflow_start, sizes_.dflow_size,
                                               world_comm);
            break;
        }
        default:
        {
            std::cout<<"ERROR : policy "<<prod_dflow_redist<<" unrecognized to select a redistribution component"
                     <<" Using the RedistCountMPI instead>"<<std::endl;
            redist_prod_dflow_ = new RedistCountMPI(sizes_.prod_start, sizes_.prod_size,
                                               sizes_.dflow_start, sizes_.dflow_size,
                                               world_comm);
            break;
        }

    }
  }
  else
  {
      std::cout<<"No Redistribution between producer and dflow needed."<<std::endl;
      redist_prod_dflow_ = NULL;
  }
  if (world_rank >= dflow_con_start && world_rank <= dflow_con_end)
  {
    dflow_con_comm_ = new Comm(world_comm, dflow_con_start, dflow_con_end, sizes_.dflow_size,
                               sizes_.con_size, sizes_.con_start - sizes_.dflow_start,
                               DECAF_DFLOW_CON_COMM);
    switch(dflow_cons_redist)
    {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            std::cout<<"Using Round for dflow -> cons"<<std::endl;
            redist_dflow_con_ = new RedistRoundMPI(sizes_.dflow_start, sizes_.dflow_size,
                                               sizes_.con_start, sizes_.con_size,
                                               world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            std::cout<<"Using Count for dflow -> cons"<<std::endl;
            redist_dflow_con_ = new RedistCountMPI(sizes_.dflow_start, sizes_.dflow_size,
                                                   sizes_.con_start, sizes_.con_size,
                                               world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            std::cout<<"Using ZCurve for dflow -> cons"<<std::endl;
            redist_dflow_con_ = new RedistZCurveMPI(sizes_.dflow_start, sizes_.dflow_size,
                                                    sizes_.con_start, sizes_.con_size,
                                               world_comm);
            break;
        }
        default:
        {
            std::cout<<"ERROR : policy "<<prod_dflow_redist<<" unrecognized to select a redistribution component."
                     <<" Using the RedistCountMPI instead."<<std::endl;
            redist_dflow_con_ = new RedistCountMPI(sizes_.dflow_start, sizes_.dflow_size,
                                                   sizes_.con_start, sizes_.con_size,
                                               world_comm);
            break;
        }

    }
  }
  else
  {
      std::cout<<"No Redistribution between dflow and cons needed."<<std::endl;
      redist_dflow_con_ = NULL;
  }


  err_ = DECAF_OK;
}

decaf::
Dataflow::~Dataflow()
{
  if (prod_dflow_comm_)
    delete prod_dflow_comm_;
  if (dflow_con_comm_)
    delete dflow_con_comm_;
  if (is_prod())
    delete prod_comm_;
  if (is_con())
    delete con_comm_;
  if (redist_dflow_con_)
    delete redist_dflow_con_;
  if (redist_prod_dflow_)
    delete redist_prod_dflow_;
}

void
decaf::
Dataflow::run()
{
  // MOVE TO THE USER DFLOW FUNCTIONS
  /*
  // runs the dataflow, only for those dataflow ranks that are disjoint from producer
  // dataflow ranks that overlap producer ranks run the dataflow as part of the put function
  if (is_dflow() && !is_prod())
  {
    // TODO: when pipelining, would not store all items in dataflow before forwarding to consumer
    // as is done below
    for (int i = 0; i < sizes_.con_nsteps; i++){
      std::cout<<"dflow forwarding"<<std::endl;
      forward();
    }
  }*/
}

void
decaf::
Dataflow::put(void* d,                // source data
              int count)              // number of datatype instances, default = 1
{
  if (d && count)                     // normal, non-null put
  {
    data_->put_items(d);
    data_->put_nitems(count);

    for (int i = 0; i < prod_dflow_comm_->num_outputs(); i++)
    {
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
  }
  else                                 // null put
  {
    for (int i = 0; i < prod_dflow_comm_->num_outputs(); i++)
      prod_dflow_comm_->put(NULL, prod_dflow_comm_->start_output() + i, DECAF_PROD);
  }

  // this rank may also serve as dataflow in case producer and dataflow overlap
  if (is_dflow())
    forward();
}

void
decaf::
Dataflow::put(std::shared_ptr<BaseData> data, TaskType role)
{
    //if(is_prod())
    if(role == DECAF_PROD)
    {
        if(redist_prod_dflow_ == NULL) std::cerr<<"Trying to access a null communicator."<<std::endl;
        redist_prod_dflow_->process(data, DECAF_REDIST_SOURCE);
    }
    //if(is_dflow())
    else if(role == DECAF_DFLOW)
    {
        if(redist_dflow_con_ == NULL) std::cerr<<"Trying to access a null communicator."<<std::endl;
        redist_dflow_con_->process(data, DECAF_REDIST_SOURCE);
    }
}

void*
decaf::
Dataflow::get(bool no_copy)
{
  if (no_copy)
    return data_->put_items();
  else
  {
    dflow_con_comm_->get(data_, DECAF_CON);
    return data_->get_items(DECAF_CON);
  }
}

void
decaf::
Dataflow::get(std::shared_ptr<BaseData> data, TaskType role)
{
    //if(is_dflow())
    if(role == DECAF_DFLOW)
    {
        if(redist_prod_dflow_ == NULL) std::cerr<<"Trying to access a null communicator."<<std::endl;
        redist_prod_dflow_->process(data, DECAF_REDIST_DEST);
    }
    //if(is_con())
    else if(role == DECAF_CON)
    {
        if(redist_dflow_con_ == NULL) std::cerr<<"Trying to access a null communicator."<<std::endl;
        redist_dflow_con_->process(data, DECAF_REDIST_DEST);
    }
}

// run the dataflow
void
decaf::
Dataflow::dataflow()
{
  // TODO: when pipelining, would not store all items in dataflow before forwarding to consumer
  // as is done below
  for (int i = 0; i < sizes_.con_nsteps; i++)
    forward();
}

// forward the data through the dataflow
void
decaf::
Dataflow::forward()
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
Dataflow::flush()
{
  if (is_prod())
  {
    prod_dflow_comm_->flush();
    redist_prod_dflow_->flush();
  }
  if (is_dflow())
  {
    dflow_con_comm_->flush();
    redist_dflow_con_->flush();
  }
  if (is_dflow() || is_con())
    data_->flush();
}

#endif
