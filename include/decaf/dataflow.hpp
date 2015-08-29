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
#include "transport/mpi/types.h"
#include "transport/mpi/comm.hpp"
#include "transport/mpi/data.hpp"
#include "transport/mpi/redist_count_mpi.h"
#include "transport/mpi/redist_round_mpi.h"
#include "transport/mpi/redist_zcurve_mpi.h"
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
             Decomposition prod_dflow_redist = DECAF_CONTIG_DECOMP,
             Decomposition dflow_cons_redist = DECAF_CONTIG_DECOMP);
    ~Dataflow();
    void put(std::shared_ptr<BaseData> data, TaskType role);
    void get(std::shared_ptr<BaseData> data, TaskType role);
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
    void forward();

  private:
    CommHandle world_comm_;     // handle to original world communicator
    Comm* prod_comm_;           // producer communicator
    Comm* con_comm_;            // consumer communicator
    RedistComp* redist_prod_dflow_;  // Redistribution component between producer and dataflow
    RedistComp* redist_dflow_con_;   // Redestribution component between a dataflow and consummer
    DecafSizes sizes_;          // sizes of communicators, time steps
    void (*pipeliner_)(Dataflow*); // user-defined pipeliner code
    void (*checker_)(Dataflow*);   // user-defined resilience code
    int err_;                   // last error
    CommType type_;             // whether this instance is producer, consumer, dataflow, or other
  };

} // namespace

decaf::
Dataflow::Dataflow(CommHandle world_comm,
                   DecafSizes& decaf_sizes,
                   void (*pipeliner)(Dataflow*),
                   void (*checker)(Dataflow*),
                   // Data* data,
                   Decomposition prod_dflow_redist,
                   Decomposition dflow_cons_redist):
  world_comm_(world_comm),
  redist_prod_dflow_(NULL),
  redist_dflow_con_(NULL),
  pipeliner_(pipeliner),
  checker_(checker),
  type_(DECAF_OTHER_COMM)
{
  // sizes is a POD struct, initialization was not allowed in C++03; used assignment workaround
  // TODO: easier now in C++11?
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
            std::cout<<"ERROR : policy "<<prod_dflow_redist<<
                " unrecognized to select a redistribution component"
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
            std::cout<<"ERROR : policy "<<prod_dflow_redist<<
                " unrecognized to select a redistribution component."
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
Dataflow::put(std::shared_ptr<BaseData> data, TaskType role)
{
    if(role == DECAF_PROD)
    {
        if(redist_prod_dflow_ == NULL) std::cerr<<"Trying to access a null communicator."<<
                                           std::endl;
        redist_prod_dflow_->process(data, DECAF_REDIST_SOURCE);
    }
    else if(role == DECAF_DFLOW)
    {
        if(redist_dflow_con_ == NULL) std::cerr<<"Trying to access a null communicator."<<
                                          std::endl;
        redist_dflow_con_->process(data, DECAF_REDIST_SOURCE);
    }
}

void
decaf::
Dataflow::get(std::shared_ptr<BaseData> data, TaskType role)
{
    if(role == DECAF_DFLOW)
    {
        if(redist_prod_dflow_ == NULL) std::cerr<<"Trying to access a null communicator."<<
                                           std::endl;
        redist_prod_dflow_->process(data, DECAF_REDIST_DEST);
    }
    else if(role == DECAF_CON)
    {
        if(redist_dflow_con_ == NULL) std::cerr<<"Trying to access a null communicator."<<
                                          std::endl;
        redist_dflow_con_->process(data, DECAF_REDIST_DEST);
    }
}

// cleanup after each time step
void
decaf::
Dataflow::flush()
{
  if (is_prod())
    redist_prod_dflow_->flush();
  if (is_dflow())
    redist_dflow_con_->flush();
}

#endif
