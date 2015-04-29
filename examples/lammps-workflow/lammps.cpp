//---------------------------------------------------------------------------
//
// lammps example
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <utility>
#include <map>

// lammps includes
#include "lammps.h"
#include "input.h"
#include "atom.h"
#include "library.h"

using namespace decaf;
using namespace LAMMPS_NS;
using namespace std;

struct lammps_args_t                             // custom args for running lammps
{
  LAMMPS* lammps;
  char infile[256];
};

struct pos_args_t                                // custom args for atom positions
{
  int natoms;                                    // number of atoms
  double* pos;                                   // atom positions
};

// user-defined pipeliner code
void pipeliner(Decaf* decaf)
{
}

// user-defined resilience code
void checker(Decaf* decaf)
{
}

// this producer runs lammps and puts the atom positions to the dataflow at the consumer intervals
void prod_l(int t_current,                    // current time step
            int t_interval,                   // consumer time interval
            int t_nsteps,                     // total number of time steps
            vector<Decaf*> decaf,             // decaf objects in user-defined order
            void* args)                       // custom args
{
  struct lammps_args_t* a = (lammps_args_t*)args; // custom args
  double* x;                                  // atom positions

  if (t_current == 0)                         // first time step
  {
    // only create lammps for first decaf instance
    a->lammps = new LAMMPS(0, NULL, decaf[0]->prod_comm_handle());
    a->lammps->input->file((char*)a->infile);
  }

  a->lammps->input->one("run 1");
  int natoms = static_cast<int>(a->lammps->atom->natoms);
  x = new double[3 * natoms];
  lammps_gather_atoms(a->lammps, (char*)"x", 1, 3, x);

  if (!((t_current + 1) % t_interval))
  {
    for (size_t i = 0; i < decaf.size(); i++)
    {
      if (decaf[i]->prod_comm()->rank() == 0) // lammps gathered all positions to rank 0
      {
        fprintf(stderr, "+ lammps producing time step %d with %d atoms\n", t_current, natoms);
        // debug
//         for (int i = 0; i < 10; i++)         // print first few atoms
//           fprintf(stderr, "%.3lf %.3lf %.3lf\n", x[3 * i], x[3 * i + 1], x[3 * i + 2]);
        decaf[i]->put(x, 3 * natoms);
      }
      else
        decaf[i]->put(NULL);                  // put is collective; all producer ranks must call it
      decaf[i]->flush();                      // need to clean up after each time step
    }
  }
  delete[] x;

  if (t_current == t_nsteps - 1)              // last time step
    delete a->lammps;
}

// this producer takes print2 data and sends it to print3
void prod_p2(int t_current,                   // current time step
             int t_interval,                  // consumer time interval
             int t_nsteps,                    // total number of time steps
             vector<Decaf*> decaf,            // decaf objects in user-defined order
             void* args)                      // custom args
{
  struct pos_args_t* a = (pos_args_t*)args;   // custom args

  if (!((t_current + 1) % t_interval))
  {
    fprintf(stderr, "+ print2 producing time step %d with %d atoms\n", t_current, a->natoms);
    decaf[0]->put(a->pos, a->natoms * 3);
    decaf[0]->flush();                      // need to clean up after each time step
  }
}

// consumer gets the atom positions and prints them
// check your modulo arithmetic to ensure you get exactly decaf->con_nsteps times
void con(int t_current,                        // current time step
         int t_interval,                       // consumer time interval
         int,                                  // total number of time steps (unused)
         vector<Decaf*> decaf,                 // decaf objects in user-defined order
         void*)                                // custom args (unused)
{
  if (!((t_current + 1) % t_interval))
  {
    double* pos    = (double*)decaf[0]->get(); // we know decaf.size() = 1 in this example
    // debug
    fprintf(stderr, "consumer print1 or print3 printing %d atoms\n", decaf[0]->get_nitems() / 3);
//     for (int i = 0; i < 10; i++)               // print first few atoms
//       fprintf(stderr, "%.3lf %.3lf %.3lf\n", pos[3 * i], pos[3 * i + 1], pos[3 * i + 2]);
    decaf[0]->flush(); // need to clean up after each time step
  }
}

// consumer gets the atom positions and copies them
// check your modulo arithmetic to ensure you get exactly decaf->con_nsteps times
void con_p2(int t_current,                     // current time step
         int t_interval,                       // consumer time interval
         int,                                  // total number of time steps (unused)
         vector<Decaf*> decaf,                 // decaf objects in user-defined order
         void* args)                           // custom args
{
  struct pos_args_t* a = (pos_args_t*)args;    // custom args
  if (!((t_current + 1) % t_interval))
  {
    double* pos = (double*)decaf[0]->get();    // we know decaf.size() = 1 in this example
    a->natoms   = decaf[0]->get_nitems() / 3;
    a->pos      = new double[a->natoms * 3];
    fprintf(stderr, "consumer print 2 copying %d atoms\n", a->natoms);
    for (int i = 0; i < a->natoms; i++)
    {
      a->pos[3 * i    ] = pos[3 * i    ];
      a->pos[3 * i + 1] = pos[3 * i + 1];
      a->pos[3 * i + 2] = pos[3 * i + 2];
    }
    decaf[0]->flush(); // need to clean up after each time step
  }
}

// dataflow just needs to flush on every time step
void dflow(int,                               // current time step (unused)
           int,                               // consumer time interval (unused)
           int,                               // total number of time steps (unused)
           vector<Decaf*> decaf,              // decaf objects in user-defined order
           void*)                             // custom args (unused)
{
  for (size_t i = 0; i < decaf.size(); i++)
    decaf[i]->flush();                        // need to clean up after each time step
}

void node_callback(WorkflowNode& node,
                   int t_current,
                   int t_interval,
                   int t_nsteps,
                   vector<Decaf*> decafs)
{
  fprintf(stderr, "node: start_proc = %d nprocs = %d\n", node.start_proc, node.nprocs);
}

void link_callback(WorkflowLink& link,
                   int t_current,
                   int t_interval,
                   int t_nsteps,
                   Decaf* decaf)
{
  fprintf(stderr, "link: prod = %d con = %d start_proc = %d nprocs = %d\n",
          link.prod, link.con, link.start_proc, link.nprocs);
}

// node and link callback functions
void lammps()
{
  fprintf(stderr, "lammps\n");
}

void print1()
{
  fprintf(stderr, "print1\n");
}

void print2_prod()
{
  fprintf(stderr, "print2_prod\n");
}

void print2_con()
{
  fprintf(stderr, "print2_con\n");
}

void print3()
{
  fprintf(stderr, "print3\n");
}

void lammps_print1()
{
  fprintf(stderr, "lammps -> print1\n");
}

void lammps_print2()
{
  fprintf(stderr, "lammps -> print2\n");
}

void print2_print3()
{
  fprintf(stderr, "print2 -> print3\n");
}

void run(Workflow& workflow,
         int prod_nsteps,
         int con_nsteps,
         char* infile)
{
  // map of callbacks
  // TODO: move to a separate function
  // this map needs to be custom made by the user custom for all possible functions to be
  // used in a workflow or a family of workflows; maybe declared to decaf during intitialization
  // would be better
  pair<char*, void(*)()> p;
  map<char*, void(*)()> callbacks;
  p = make_pair((char*)"lammps"       , &lammps        ); callbacks.insert(p);
  p = make_pair((char*)"print1"       , &print1        ); callbacks.insert(p);
  p = make_pair((char*)"print2_prod"  , &print2_prod   ); callbacks.insert(p);
  p = make_pair((char*)"print2_con"   , &print2_con    ); callbacks.insert(p);
  p = make_pair((char*)"print3"       , &print3        ); callbacks.insert(p);
  p = make_pair((char*)"lammps_print1", &lammps_print1 ); callbacks.insert(p);
  p = make_pair((char*)"lammps_print2", &lammps_print2 ); callbacks.insert(p);
  p = make_pair((char*)"print2_print3", &print2_print3 ); callbacks.insert(p);

  DecafSizes decaf_sizes;
  vector<Decaf*> decafs;

  MPI_Init(NULL, NULL);

  // define the data type
  Data data(MPI_DOUBLE);

  // create decaf instances for all links in the workflow
  for (size_t i = 0; i < workflow.links.size(); i++)
  {
    int p = workflow.links[i].prod;
    int c = workflow.links[i].con;
    decaf_sizes.prod_size   = workflow.nodes[p].nprocs;
    decaf_sizes.dflow_size  = workflow.links[i].nprocs;
    decaf_sizes.con_size    = workflow.nodes[c].nprocs;
    decaf_sizes.prod_start  = workflow.nodes[p].start_proc;
    decaf_sizes.dflow_start = workflow.links[i].start_proc;
    decaf_sizes.con_start   = workflow.nodes[c].start_proc;
    decaf_sizes.con_nsteps  = con_nsteps;
    decafs.push_back(new Decaf(MPI_COMM_WORLD,
                               decaf_sizes,
                               &pipeliner,
                               &checker,
                               &data));
    decafs[i]->err();
  }

  // TODO: assuming the same consumer interval for the entire workflow
  // this should not be necessary, need to experiment
  int con_interval;                                      // consumer interval
  if (decaf_sizes.con_nsteps)
    con_interval = prod_nsteps / decaf_sizes.con_nsteps; // consume every so often
  else
    con_interval = -1;                                   // don't consume

  lammps_args_t lammps_args;                             // custom args for lammps
  strcpy(lammps_args.infile, infile);
  pos_args_t pos_args;                                   // custom args for atom positions
  pos_args.pos = NULL;

  // find the sources of the workflow
  vector<int> sources;
  for (size_t i = 0; i < workflow.nodes.size(); i++)
  {
    if (workflow.nodes[i].in_links.size() == 0)
      sources.push_back(i);
  }

  // compute a BFS of the graph
  vector<int> bfs_tree;
  bfs(workflow, sources, bfs_tree);

  // debug: print the bfs order
//   for (int i = 0; i < bfs_tree.size(); i++)
//     fprintf(stderr, "bfs[%d] = %d\n", i, bfs_tree[i]);

  // TODO: start the dataflows once the callbacks are actually moving information
  // will deadlock until then

  // start the dataflows
//   for (size_t i = 0; i < workflow.links.size(); i++)
//     decafs[i]->run();

  // run the main loop
  for (int t = 0; t < prod_nsteps; t++)
  {

    // execute the workflow, calling nodes in BFS order
    for (size_t i = 0; i < bfs_tree.size(); i++)
    {
      int u = bfs_tree[i];

      // fill decafs
      vector<Decaf*> out_decafs;
      vector<Decaf*> in_decafs;
      for (size_t j = 0; j < workflow.nodes[u].out_links.size(); j++)
      {
//         fprintf(stderr, "out_links[%lu] = %d\n", j, workflow.nodes[u].out_links[j]);
        out_decafs.push_back(decafs[workflow.nodes[u].out_links[j]]);
      }
      for (size_t j = 0; j < workflow.nodes[u].in_links.size(); j++)
      {
//         fprintf(stderr, "in_links[%lu] = %d\n", j, workflow.nodes[u].in_links[j]);
        in_decafs.push_back(decafs[workflow.nodes[u].in_links[j]]);
      }

//       // producer
//       if (out_decafs.size() && out_decafs[0]->is_prod())
// //         task(workflow.nodes[u].prod_func, workflow.nodes[u].prod_args, t,
// //              con_interval, prod_nsteps, out_decafs);
//         callbacks[workflow.nodes[u].prod_func]();

//       // consumer
//       if (in_decafs.size() && in_decafs[0]->is_con())
// //         task(workflow.nodes[u].con_func, workflow.nodes[u].con_args, t,
// //              con_interval, prod_nsteps, in_decafs);
//         callbacks[workflow.nodes[u].con_func]();

      // dataflow
      for (size_t j = 0; j < out_decafs.size(); j++)
      {
        int l = workflow.nodes[u].out_links[j];
        if (out_decafs[j]->is_dflow())
        {
          //           task(workflow.links[l].dflow_func, workflow.links[l].dflow_args, t,
          //                con_interval, prod_nsteps, out_decafs[j]);
          for (int i = 0; i < strlen(workflow.links[l].dflow_func) + 1; i++)
            fprintf(stderr, "%d ", workflow.links[l].dflow_func[i]);
          fprintf(stderr, "\n");
          fprintf(stderr, "count[%s] = %d\n", workflow.links[l].dflow_func,
                  callbacks.count(workflow.links[l].dflow_func));
          //           callbacks[workflow.links[l].dflow_func]();
        }
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

  MPI_Finalize();
}

int main(int argc,
         char** argv)
{
  // normal entry point is run(), called by python
  // main is a test driver for debugging purposes

  Workflow workflow;
  int prod_nsteps = 1;
  int con_nsteps = 1;
  char infile[256];
  strcpy(infile, "/Users/tpeterka/software/decaf/examples/lammps-workflow/in.melt");
  vector<void (*)()> node_funcs;
  vector<void (*)()> link_funcs;

  // fill workflow nodes
  WorkflowNode node;
  node.in_links.push_back(1);                    // print1
  node.start_proc = 5;
  node.nprocs = 1;
  node.prod_func = NULL;
  node.con_func = (char*)"print1";
  workflow.nodes.push_back(node);

  node.out_links.clear();
  node.in_links.clear();
  node.in_links.push_back(0);                    // print3
  node.start_proc = 9;
  node.nprocs = 1;
  node.prod_func = NULL;
  node.con_func = (char*)"print3";
  workflow.nodes.push_back(node);

  node.out_links.clear();
  node.in_links.clear();
  node.out_links.push_back(0);                   // print2
  node.in_links.push_back(2);
  node.start_proc = 7;
  node.nprocs = 1;
  node.prod_func = (char*)"print2_prod";
  node.con_func = (char*)"print2_con";
  workflow.nodes.push_back(node);

  node.out_links.clear();
  node.in_links.clear();
  node.out_links.push_back(1);                   // lammps
  node.out_links.push_back(2);
  node.start_proc = 0;
  node.nprocs = 4;
  node.prod_func = (char*)"lammps";
  node.con_func = NULL;
  workflow.nodes.push_back(node);

  // fill workflow links
  WorkflowLink link;
  link.prod = 2;                                 // print2 - print3
  link.con = 1;
  link.start_proc = 8;
  link.nprocs = 1;
  link.dflow_func = (char*)"print2_print3";
  workflow.links.push_back(link);

  link.prod = 3;                                 // lammps - print1
  link.con = 0;
  link.start_proc = 4;
  link.nprocs = 1;
  link.dflow_func = (char*)"lammps_print1";
  workflow.links.push_back(link);

  link.prod = 3;                                 // lammps - print2
  link.con = 2;
  link.start_proc = 6;
  link.nprocs = 1;
  link.dflow_func = (char*)"lammps_print2";
  workflow.links.push_back(link);

  // run decaf
  run(workflow, prod_nsteps, con_nsteps, infile);

  return 0;
}
