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

// lammps includes
#include "lammps.h"
#include "input.h"
#include "atom.h"
#include "library.h"

using namespace decaf;
using namespace LAMMPS_NS;

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

// gets command line args
void GetArgs(int argc,
             char **argv,
             DecafSizes& decaf_sizes,
             int& prod_nsteps,
             char* infile)
{
  assert(argc >= 9);

  decaf_sizes.prod_size    = atoi(argv[1]);
  decaf_sizes.dflow_size   = atoi(argv[2]);
  decaf_sizes.con_size     = atoi(argv[3]);

  decaf_sizes.prod_start   = atoi(argv[4]);
  decaf_sizes.dflow_start  = atoi(argv[5]);
  decaf_sizes.con_start    = atoi(argv[6]);

  prod_nsteps              = atoi(argv[7]);   // user's, not decaf's variable
  decaf_sizes.con_nsteps   = atoi(argv[8]);
  strcpy(infile, argv[9]);
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

void run(DecafSizes& decaf_sizes,
         int prod_nsteps,
         char* infile)
{
  MPI_Init(NULL, NULL);

  // define the data type
  Data data(MPI_DOUBLE);

  // hard-coding a little 4-node workflow with 3 decaf instances (one per link)
  //         print1 (1 proc)
  //       /
  //   lammps (4 procs)
  //       \
  //         print2 (1 proc) - print3 (1 proc)
  //
  // entire workflow takes 10 procs (1 dataflow proc between each producer consumer pair)
  // dataflow can be overallped, but currently all disjoint procs (simplest case)

  // lammps -  print1
  decaf_sizes.prod_size   = 4;
  decaf_sizes.dflow_size  = 1;
  decaf_sizes.con_size    = 1;
  decaf_sizes.prod_start  = 0;
  decaf_sizes.dflow_start = 4;
  decaf_sizes.con_start   = 5;
  decaf_sizes.con_nsteps  = 1;
  Decaf* decaf_lp1 = new Decaf(MPI_COMM_WORLD,
                               decaf_sizes,
                               &pipeliner,
                               &checker,
                               &data);
  decaf_lp1->err();

  // lammps -  print2
  decaf_sizes.prod_size   = 4;
  decaf_sizes.dflow_size  = 1;
  decaf_sizes.con_size    = 1;
  decaf_sizes.prod_start  = 0;
  decaf_sizes.dflow_start = 6;
  decaf_sizes.con_start   = 7;
  decaf_sizes.con_nsteps  = 1;
  Decaf* decaf_lp2 = new Decaf(MPI_COMM_WORLD,
                               decaf_sizes,
                               &pipeliner,
                               &checker,
                               &data);
  decaf_lp2->err();

  // print2 -  print3
  decaf_sizes.prod_size   = 1;
  decaf_sizes.dflow_size  = 1;
  decaf_sizes.con_size    = 1;
  decaf_sizes.prod_start  = 7;
  decaf_sizes.dflow_start = 8;
  decaf_sizes.con_start   = 9;
  decaf_sizes.con_nsteps  = 1;
  Decaf* decaf_p2p3 = new Decaf(MPI_COMM_WORLD,
                                decaf_sizes,
                                &pipeliner,
                                &checker,
                                &data);
  decaf_p2p3->err();

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

  // start the dataflows
  decaf_lp1->run();
  decaf_lp2->run();
  decaf_p2p3->run();

  // run the main loop
  for (int t = 0; t < prod_nsteps; t++)
  {
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

  // cleanup
  delete decaf_lp1;
  delete decaf_lp2;
  delete decaf_p2p3;
  MPI_Finalize();
}

int main(int argc,
         char** argv)
{
  // parse command line args
  DecafSizes decaf_sizes;
  int prod_nsteps;
  char infile[256];
  GetArgs(argc, argv, decaf_sizes, prod_nsteps, infile);

  // run decaf
  run(decaf_sizes, prod_nsteps, infile);

  return 0;
}
