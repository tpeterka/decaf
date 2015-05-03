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

#include <map>

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
          void (*pipeliner)(Decaf*),
          void (*checker)(Decaf*),
          Data* data);
    ~Decaf();
    void run();
    void put(void* d, int count = 1);
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
    CommHandle prod_comm_handle() { return prod_comm_->handle(); }
    CommHandle con_comm_handle()  { return con_comm_->handle();  }
    Comm* prod_comm()             { return prod_comm_; }
    Comm* con_comm()              { return con_comm_;  }

  private:
    CommHandle world_comm_;     // handle to original world communicator
    Comm* prod_comm_;           // producer communicator
    Comm* con_comm_;            // consumer communicator
    Comm* prod_dflow_comm_;     // communicator covering producer and dataflow
    Comm* dflow_con_comm_;      // communicator covering dataflow and consumer
    Data* data_;                // data model
    DecafSizes sizes_;          // sizes of communicators, time steps
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
             void (*pipeliner)(Decaf*),
             void (*checker)(Decaf*),
             Data* data):
  world_comm_(world_comm),
  prod_dflow_comm_(NULL),
  dflow_con_comm_(NULL),
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
    prod_dflow_comm_ = new Comm(world_comm, prod_dflow_start, prod_dflow_end, sizes_.prod_size,
                                sizes_.dflow_size, sizes_.dflow_start - sizes_.prod_start,
                                DECAF_PROD_DFLOW_COMM);
  if (world_rank >= dflow_con_start && world_rank <= dflow_con_end)
    dflow_con_comm_ = new Comm(world_comm, dflow_con_start, dflow_con_end, sizes_.dflow_size,
                               sizes_.con_size, sizes_.con_start - sizes_.dflow_start,
                               DECAF_DFLOW_CON_COMM);

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
Decaf::run()
{
  // runs the dataflow, only for those dataflow ranks that are disjoint from producer
  // dataflow ranks that overlap producer ranks run the dataflow as part of the put function
  if (is_dflow() && !is_prod())
  {
    // TODO: when pipelining, would not store all items in dataflow before forwarding to consumer
    // as is done below
    for (int i = 0; i < sizes_.con_nsteps; i++)
      forward();
  }
}

void
decaf::
Decaf::put(void* d,                   // source data
           int count)                 // number of datatype instances, default = 1
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

void
BuildDecafs(Workflow& workflow,
            vector<decaf::Decaf*>& decafs,
            int con_nsteps,
            CommHandle world_comm,
            void (*pipeliner)(decaf::Decaf*),
            void (*checker)(decaf::Decaf*),
            decaf::Data* data)
{
  DecafSizes decaf_sizes;
  for (size_t i = 0; i < workflow.links.size(); i++)
  {
    int prod  = workflow.links[i].prod;    // index into workflow nodes
    int dflow = i;                         // index into workflow links
    int con   = workflow.links[i].con;     // index into workflow nodes
    decaf_sizes.prod_size   = workflow.nodes[prod].nprocs;
    decaf_sizes.dflow_size  = workflow.links[dflow].nprocs;
    decaf_sizes.con_size    = workflow.nodes[con].nprocs;
    decaf_sizes.prod_start  = workflow.nodes[prod].start_proc;
    decaf_sizes.dflow_start = workflow.links[dflow].start_proc;
    decaf_sizes.con_start   = workflow.nodes[con].start_proc;
    decaf_sizes.con_nsteps  = con_nsteps;
    decafs.push_back(new decaf::Decaf(world_comm, decaf_sizes, pipeliner, checker, data));
    decafs[i]->err();
  }
}

// BFS node
struct BFSNode
{
  int index;                    // index of node in all workflow nodes
  int dist;                     // distance from start (level in bfs tree)
};

// computes breadth-first search (BFS) traversal of the workflow nodes
// returns number of levels in the bfs tree (distance of farthest node from source)
int
bfs(Workflow& workflow,         // the workflow
    vector<int>& sources,       // starting nodes (indices into workflow nodes)
    vector<BFSNode>& bfs_nodes) // output bfs node order (indices into workflow nodes)
{
  vector<bool> visited_nodes;
  visited_nodes.reserve(workflow.nodes.size());
  queue<BFSNode> q;
  BFSNode bfs_node;

  // init
  bfs_node.index = -1;
  bfs_node.dist  = 0;
  for (size_t i = 0; i < workflow.nodes.size(); i++)
    visited_nodes[i] = false;

  // init source nodes
  for (size_t i = 0; i < sources.size(); i++)
  {
    visited_nodes[sources[i]] = true;
    bfs_node.index = sources[i];
    bfs_node.dist = 0;
    q.push(bfs_node);
  }

  while (!q.empty())
  {
    int u = q.front().index;
    int d = q.front().dist;
    q.pop();
    bfs_node.index = u;
    bfs_node.dist = d;
    bfs_nodes.push_back(bfs_node);
    for (size_t i = 0; i < workflow.nodes[u].out_links.size(); i++)
    {
      int v = workflow.links[workflow.nodes[u].out_links[i]].con;
      if (visited_nodes[v] == false)
      {
        visited_nodes[v] = true;
        bfs_node.index = v;
        bfs_node.dist = d + 1;
        q.push(bfs_node);
      }
    }
  }
  return bfs_node.dist;
}

void run_workflow(Workflow& workflow,                     // the workflow
                  int prod_nsteps,                        // number of producer time steps
                  int con_nsteps,                         // number of consumer time steps
                  decaf::Data* data,                      // data model
                  map<string, void(*)(void*,
                                      int,
                                      int,
                                      int,
                                      vector<decaf::Decaf*>&,
                                      int)> callbacks,    // map of callback functions
                  CommHandle world_comm,                  // world communicator
                  void (*pipeliner)(decaf::Decaf*),       // custom pipeliner code
                  void (*checker)(decaf::Decaf*))         // custom resilience code
{
  vector<decaf::Decaf*> decafs;

  // create decaf instances for all links in the workflow
  BuildDecafs(workflow, decafs, con_nsteps, world_comm, pipeliner, checker, data);

  // TODO: assuming the same consumer interval for the entire workflow
  // this should not be necessary, need to experiment
  int con_interval;                                      // consumer interval
  if (con_nsteps)
    con_interval = prod_nsteps / con_nsteps;             // consume every so often
  else
    con_interval = -1;                                   // don't consume

  // find the sources of the workflow
  vector<int> sources;
  for (size_t i = 0; i < workflow.nodes.size(); i++)
  {
    if (workflow.nodes[i].in_links.size() == 0)
      sources.push_back(i);
  }

  // compute a BFS of the graph
  vector<BFSNode> bfs_order;
  int nlevels = bfs(workflow, sources, bfs_order);

  // debug: print the bfs order
//   for (int i = 0; i < bfs_order.size(); i++)
//     fprintf(stderr, "nlevels = %d bfs[%d] = index %d dist %d\n",
//             nlevels, i, bfs_order[i].index, bfs_order[i].dist);

  // start the dataflows
  for (size_t i = 0; i < workflow.links.size(); i++)
    decafs[i]->run();

  // run the main loop
  for (int t = 0; t < prod_nsteps; t++)
  {

    // execute the workflow, calling nodes in BFS order
    int n = 0;
    for (int level = 0; level <= nlevels; level++)
    {
      while (1)
      {
        if (n >= bfs_order.size() || bfs_order[n].dist > level)
          break;
        int u = bfs_order[n].index;
        fprintf(stderr, "level = %d n = %d u = %d\n", level, n, u);

        // fill decafs
        vector<decaf::Decaf*> out_decafs;
        vector<decaf::Decaf*> in_decafs;
        for (size_t j = 0; j < workflow.nodes[u].out_links.size(); j++)
          out_decafs.push_back(decafs[workflow.nodes[u].out_links[j]]);
        for (size_t j = 0; j < workflow.nodes[u].in_links.size(); j++)
          in_decafs.push_back(decafs[workflow.nodes[u].in_links[j]]);

        // consumer
        if (in_decafs.size() && in_decafs[0]->is_con())
          callbacks[workflow.nodes[u].con_func](workflow.nodes[u].con_args, t,
                                                con_interval, prod_nsteps, in_decafs, -1);
        // producer
        if (out_decafs.size() && out_decafs[0]->is_prod())
          callbacks[workflow.nodes[u].prod_func](workflow.nodes[u].prod_args, t,
                                                 con_interval, prod_nsteps, out_decafs, -1);

        // dataflow
        for (size_t j = 0; j < out_decafs.size(); j++)
        {
          int l = workflow.nodes[u].out_links[j];
          if (out_decafs[j]->is_dflow())
            callbacks[workflow.links[l].dflow_func](workflow.links[l].dflow_args, t,
                                                    con_interval, prod_nsteps, out_decafs, j);
        }
        n++;
      }
    }
  }

#if 0
    // both lammps - print1 and lammps - print2
    if (decaf_lp1->is_prod())
    {
      vector<Decaf*> decafs_lp1_lp2;
      decafs_lp1_lp2.push_back(decaf_lp1);
      decafs_lp1_lp2.push_back(decaf_lp2);
      prod_l(t, con_interval, prod_nsteps, decafs_lp1_lp2, &lammps_args);
    }

    // lammps - print1
    vector<Decaf*> decafs_lp1;
    decafs_lp1.push_back(decaf_lp1);
    if (decaf_lp1->is_con())
      con(t, con_interval, prod_nsteps, decafs_lp1, &pos_args);
    if (decaf_lp1->is_dflow())
      dflow(t, con_interval, prod_nsteps, decafs_lp1, NULL);

    // lammps - print2
    vector<Decaf*> decafs_lp2;
    decafs_lp2.push_back(decaf_lp2);
    if (decaf_lp2->is_con())
      con_p2(t, con_interval, prod_nsteps, decafs_lp2, &pos_args);
    if (decaf_lp2->is_dflow())
      dflow(t, con_interval, prod_nsteps, decafs_lp2, NULL);

    // print2 - print3
    vector<Decaf*> decafs_p2p3;
    decafs_p2p3.push_back(decaf_p2p3);
    if (decaf_p2p3->is_prod())
      prod_p2(t, con_interval, prod_nsteps, decafs_p2p3, &pos_args);
    if (decaf_p2p3->is_con())
      con(t, con_interval, prod_nsteps, decafs_p2p3, &pos_args);
    if (decaf_p2p3->is_dflow())
      dflow(t, con_interval, prod_nsteps, decafs_p2p3, NULL);
    if (pos_args.pos)
      delete[] pos_args.pos;
  }
#endif

  // cleanup
  for (size_t i = 0; i < decafs.size(); i++)
    delete decafs[i];
}

#endif
