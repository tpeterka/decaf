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

struct args_t                                // custom args for prod, con, dflow
{
  LAMMPS* lammps;
  char infile[256];
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

  prod_nsteps              = atoi(argv[7]);  // user's, not decaf's variable
  decaf_sizes.con_nsteps   = atoi(argv[8]);
  strcpy(infile, argv[9]);
}

// producer runs lammps and puts the atom positions to the dataflow at the consumer intervals
void prod(int t_current,                    // current time step
          int t_interval,                   // consumer time interval
          int t_nsteps,                     // total number of time steps
          Decaf* decaf,                     // decaf object
          void* args)                       // custom args
{
  struct args_t* a = (args_t*)args;          // custom args
  double* x;                                 // atom positions

  if (t_current == 0)                        // first time step
  {
    a->lammps = new LAMMPS(0, NULL, decaf->prod_comm_handle());
    a->lammps->input->file((char*)a->infile);
  }

  a->lammps->input->one("run 1");
  int natoms = static_cast<int>(a->lammps->atom->natoms);
  x = new double[3 * natoms];
  lammps_gather_atoms(a->lammps, (char*)"x", 1, 3, x);

  if (!((t_current + 1) % t_interval))
  {
    if (decaf->prod_comm()->rank() == 0)     // lammps gathered all positions to rank 0
    {
      fprintf(stderr, "+ producing time step %d\n", t_current);
      // debug
      //           fprintf(stderr, "num atoms = %d\n", natoms);
      //           for (int i = 0; i < natoms; i++)
      //             fprintf(stderr, "%.3lf %.3lf %.3lf\n", x[3 * i], x[3 * i + 1], x[3 * i + 2]);
      decaf->put(x, 3 * natoms);
    }
    else
      decaf->put(NULL);                      // put is collective; all producer ranks must call it
  }
  decaf->flush();                            // need to clean up after each time step
  delete[] x;

  if (t_current == t_nsteps - 1)             // last time step
    delete a->lammps;
}

// consumer gets the atom positions and prints them
// check your modulo arithmetic to ensure you get exactly decaf->con_nsteps times
void con(int t_current,                      // current time step
         int t_interval,                     // consumer time interval
         int,                                // total number of time steps (unused)
         Decaf* decaf,                       // decaf object
         void*)                              // custom args (unused)
{
  if (!((t_current + 1) % t_interval))
  {
    double* pos = (double*)decaf->get();
    // debug
    //       fprintf(stderr, "num atoms = %d\n", decaf->get_nitems() / 3);
    //       for (int i = 0; i < decaf->get_nitems() / 3; i++)
    //         fprintf(stderr, "%.3lf %.3lf %.3lf\n", pos[3 * i], pos[3 * i + 1], pos[3 * i + 2]);
    decaf->flush(); // need to clean up after each time step
  }
}

// dataflow just needs to flush on every time step
void dflow(int,                               // current time step (unused)
           int,                               // consumer time interval (unused)
           int,                               // total number of time steps (unused)
           Decaf* decaf,                      // decaf object
           void*)                             // custom args (unused)
{
  decaf->flush();                             // need to clean up after each time step
}

void run(DecafSizes& decaf_sizes,
         int prod_nsteps,
         char* infile)
{
  MPI_Init(NULL, NULL);

  // define the data type
  Data data(MPI_DOUBLE);

  // start decaf, allocate on the heap instead of on the stack so that it can be deleted
  // before MPI_Finalize is called at the end
  Decaf* decaf = new Decaf(MPI_COMM_WORLD,
                           decaf_sizes,
                           &pipeliner,
                           &checker,
                           &data);
  decaf->err();

  int con_interval;                                      // consumer interval
  if (decaf_sizes.con_nsteps)
    con_interval = prod_nsteps / decaf_sizes.con_nsteps; // consume every so often
  else
    con_interval = -1;                                   // don't consume

  args_t args;                                           // custom args for prod, con, dflow
  strcpy(args.infile, infile);

  // run the main loop
  for (int t = 0; t < prod_nsteps; t++)
  {
    // producer
    if (decaf->is_prod())
      prod(t, con_interval, prod_nsteps, decaf, &args);

    // consumer
    if (decaf->is_con())
      con(t, con_interval, prod_nsteps, decaf, &args);

    // dataflow
    if (decaf->is_dflow())
      dflow(t, con_interval, prod_nsteps, decaf, &args);
  }

  // cleanup
  delete decaf;
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
