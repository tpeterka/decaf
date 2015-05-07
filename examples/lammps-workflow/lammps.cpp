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
  string infile;
};

struct pos_args_t                                // custom args for atom positions
{
  int natoms;                                    // number of atoms
  double* pos;                                   // atom positions
};

// user-defined pipeliner code
void pipeliner(Dataflow* dataflow)
{
}

// user-defined resilience code
void checker(Dataflow* dataflow)
{
}

// node and link callback functions

// runs lammps and puts the atom positions to the dataflow at the consumer intervals
// check your modulo arithmetic to ensure you get exactly con_nsteps times
void lammps(void* args,             // arguments to the callback
            int t_current,          // current time step
            int t_interval,         // consumer time interval
            int t_nsteps,           // total number of time steps
            vector<Dataflow*>& dataflows, // all dataflows (for producer or consumer)
            int this_dataflow = -1)    // index of one dataflow in list of all

{
  fprintf(stderr, "lammps\n");
  struct lammps_args_t* a = (lammps_args_t*)args; // custom args
  double* x;                                  // atom positions

  if (t_current == 0)                         // first time step
  {
    // only create lammps for first dataflow instance
    a->lammps = new LAMMPS(0, NULL, dataflows[0]->prod_comm_handle());
    a->lammps->input->file(a->infile.c_str());
  }

  a->lammps->input->one("run 1");
  int natoms = static_cast<int>(a->lammps->atom->natoms);
  x = new double[3 * natoms];
  lammps_gather_atoms(a->lammps, (char*)"x", 1, 3, x);

  if (!((t_current + 1) % t_interval))
  {
    for (size_t i = 0; i < dataflows.size(); i++)
    {
      if (dataflows[i]->prod_comm()->rank() == 0) // lammps gathered all positions to rank 0
      {
        fprintf(stderr, "+ lammps producing time step %d with %d atoms\n", t_current, natoms);
        // debug
//         for (int i = 0; i < 10; i++)         // print first few atoms
//           fprintf(stderr, "%.3lf %.3lf %.3lf\n", x[3 * i], x[3 * i + 1], x[3 * i + 2]);
        dataflows[i]->put(x, 3 * natoms);
      }
      else
        dataflows[i]->put(NULL);          // put is collective; all producer ranks must call it
      dataflows[i]->flush();              // need to clean up after each time step
    }
  }
  delete[] x;

  if (t_current == t_nsteps - 1)              // last time step
    delete a->lammps;
}

// gets the atom positions and prints them
// check your modulo arithmetic to ensure you get exactly con_nsteps times
void print(void* args,             // arguments to the callback
           int t_current,          // current time step
           int t_interval,         // consumer time interval
           int t_nsteps,           // total number of time steps
           vector<Dataflow*>& dataflows, // all dataflows (for producer or consumer)
           int this_dataflow = -1)    // index of one dataflow in list of all
{
  fprintf(stderr, "print\n");
  if (!((t_current + 1) % t_interval))
  {
    double* pos    = (double*)dataflows[0]->get(); // we know dataflow.size() = 1 in this example
    // debug
    fprintf(stderr, "consumer print1 or print3 printing %d atoms\n",
            dataflows[0]->get_nitems() / 3);
//     for (int i = 0; i < 10; i++)               // print first few atoms
//       fprintf(stderr, "%.3lf %.3lf %.3lf\n", pos[3 * i], pos[3 * i + 1], pos[3 * i + 2]);
    dataflows[0]->flush(); // need to clean up after each time step
  }
}

// puts the atom positions to the dataflow
// check your modulo arithmetic to ensure you get exactly con_nsteps times
void print2_prod(void* args,             // arguments to the callback
                 int t_current,          // current time step
                 int t_interval,         // consumer time interval
                 int t_nsteps,           // total number of time steps
                 vector<Dataflow*>& dataflows, // all dataflows (for producer or consumer)
                 int this_datafow = -1)    // index of one dataflow in list of all
{
  fprintf(stderr, "print2_prod\n");
  struct pos_args_t* a = (pos_args_t*)args;   // custom args

  if (!((t_current + 1) % t_interval))
  {
    fprintf(stderr, "+ print2 producing time step %d with %d atoms\n", t_current, a->natoms);
    dataflows[0]->put(a->pos, a->natoms * 3);
    dataflows[0]->flush();                      // need to clean up after each time step
    delete[] a->pos;
  }
}

// gets the atom positions and copies them
// check your modulo arithmetic to ensure you get exactly con_nsteps times
void print2_con(void* args,             // arguments to the callback
                int t_current,          // current time step
                int t_interval,         // consumer time interval
                int t_nsteps,           // total number of time steps
                vector<Dataflow*>& dataflows, // all dataflows (for producer or consumer)
                int this_dataflow = -1)    // index of one dataflow in list of all
{
  fprintf(stderr, "print2_con\n");
  struct pos_args_t* a = (pos_args_t*)args;    // custom args
  if (!((t_current + 1) % t_interval))
  {
    double* pos = (double*)dataflows[0]->get();   // we know dataflows.size() = 1 in this example
    a->natoms   = dataflows[0]->get_nitems() / 3;
    a->pos      = new double[a->natoms * 3];
    fprintf(stderr, "consumer print 2 copying %d atoms\n", a->natoms);
    for (int i = 0; i < a->natoms; i++)
    {
      a->pos[3 * i    ] = pos[3 * i    ];
      a->pos[3 * i + 1] = pos[3 * i + 1];
      a->pos[3 * i + 2] = pos[3 * i + 2];
    }
    dataflows[0]->flush(); // need to clean up after each time step
  }
}

// dataflow just needs to flush on every time step
void dflow(void* args,                   // arguments to the callback
           int t_current,                // current time step
           int t_interval,               // consumer time interval
           int t_nsteps,                 // total number of time steps
           vector<Dataflow*>& dataflows, // all dataflows
           int this_dataflow = -1)       // index of one dataflow in list of all
{
  fprintf(stderr, "dflow\n");
  for (size_t i = 0; i < dataflows.size(); i++)
    dataflows[i]->flush();               // need to clean up after each time step
}

void run(Workflow& workflow,             // workflow
         int prod_nsteps,                // number of producer time steps
         int con_nsteps,                 // number of consumer time steps
         string infile)                  // lammps input config file
{
  // map of callback functions used in a workflow or a family of workflows
  pair<string, void(*)(void*, int, int, int, vector<Dataflow*>&, int)> p;
  map<string,  void(*)(void*, int, int, int, vector<Dataflow*>&, int)> callbacks;
  p = make_pair("lammps"     , &lammps     ); callbacks.insert(p);
  p = make_pair("print"      , &print      ); callbacks.insert(p);
  p = make_pair("print2_prod", &print2_prod); callbacks.insert(p);
  p = make_pair("print2_con" , &print2_con ); callbacks.insert(p);
  p = make_pair("dflow"      , &dflow      ); callbacks.insert(p);

  // callback args
  lammps_args_t lammps_args;              // custom args for lammps
  lammps_args.infile = infile;
  pos_args_t pos_args;                    // custom args for atom positions
  pos_args.pos = NULL;
  for (size_t i = 0; i < workflow.nodes.size(); i++)
  {
    if (workflow.nodes[i].prod_func == "lammps")
      workflow.nodes[i].prod_args = &lammps_args;
    if (workflow.nodes[i].prod_func == "print2_prod")
      workflow.nodes[i].prod_args = &pos_args;
    if (workflow.nodes[i].con_func  == "print2_con"|| workflow.nodes[i].con_func  == "print")
      workflow.nodes[i].con_args  = &pos_args;
  }

  MPI_Init(NULL, NULL);

  // create and run decaf
  Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow, prod_nsteps, con_nsteps);
  Data data(MPI_DOUBLE);
  decaf->run(&data, callbacks, &pipeliner, &checker);

  // cleanup
  delete decaf;
  MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
  Workflow workflow;
  int prod_nsteps = 1;
  int con_nsteps = 1;
  string infile;
  infile = "/Users/tpeterka/software/decaf/examples/lammps-workflow/in.melt";

  // fill workflow nodes
  WorkflowNode node;
  node.in_links.push_back(1);                    // print1
  node.start_proc = 5;
  node.nprocs = 1;
  node.prod_func = "";
  node.con_func = "print";
  workflow.nodes.push_back(node);

  node.out_links.clear();
  node.in_links.clear();
  node.in_links.push_back(0);                    // print3
  node.start_proc = 9;
  node.nprocs = 1;
  node.prod_func = "";
  node.con_func = "print";
  workflow.nodes.push_back(node);

  node.out_links.clear();
  node.in_links.clear();
  node.out_links.push_back(0);                   // print2
  node.in_links.push_back(2);
  node.start_proc = 7;
  node.nprocs = 1;
  node.prod_func = "print2_prod";
  node.con_func = "print2_con";
  workflow.nodes.push_back(node);

  node.out_links.clear();
  node.in_links.clear();
  node.out_links.push_back(1);                   // lammps
  node.out_links.push_back(2);
  node.start_proc = 0;
  node.nprocs = 4;
  node.prod_func = "lammps";
  node.con_func = "";
  workflow.nodes.push_back(node);

  // fill workflow links
  WorkflowLink link;
  link.prod = 2;                                 // print2 - print3
  link.con = 1;
  link.start_proc = 8;
  link.nprocs = 1;
  link.dflow_func = "dflow";
  workflow.links.push_back(link);

  link.prod = 3;                                 // lammps - print1
  link.con = 0;
  link.start_proc = 4;
  link.nprocs = 1;
  link.dflow_func = "dflow";
  workflow.links.push_back(link);

  link.prod = 3;                                 // lammps - print2
  link.con = 2;
  link.start_proc = 6;
  link.nprocs = 1;
  link.dflow_func = "dflow";
  workflow.links.push_back(link);

  // run decaf
  run(workflow, prod_nsteps, con_nsteps, infile);

  return 0;
}
