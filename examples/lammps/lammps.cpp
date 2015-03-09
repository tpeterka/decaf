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

void run(DecafSizes& decaf_sizes,
         int prod_nsteps,
         char* infile)
{
  LAMMPS *lammps;
  MPI_Init(NULL, NULL);
  double* x;                                             // atom positions, used in producer only

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

  // init producer
  if (decaf->is_prod())
  {
    lammps = new LAMMPS(0, NULL, decaf->prod_comm_handle());
    lammps->input->file(infile);
  }

  // run the main loop
  for (int t = 0; t < prod_nsteps; t++)
  {
    // producer
    // runs lammps and puts the atom positions to the dataflow at the consumer intervals
    if (decaf->is_prod())
    {
      lammps->input->one("run 1");
      int natoms = static_cast<int>(lammps->atom->natoms);
      x = new double[3 * natoms];
      lammps_gather_atoms(lammps, (char*)"x", 1, 3, x);

      if (!((t + 1) % con_interval))
      {
        if (decaf->prod_comm()->rank() == 0) // lammps gathered all positions to rank 0
        {
          fprintf(stderr, "+ producing time step %d\n", t);
          // debug
//           fprintf(stderr, "num atoms = %d\n", natoms);
//           for (int i = 0; i < natoms; i++)
//             fprintf(stderr, "%.3lf %.3lf %.3lf\n", x[3 * i], x[3 * i + 1], x[3 * i + 2]);
          decaf->put(x, 3 * natoms);
        }
        else
          decaf->put(NULL);                 // put is collective; all producer ranks must call it
      }
    }

    // consumer
    // gets the atom positions and prints them
    // check your modulo arithmetic to ensure you get exactly decaf->con_nsteps times
    if (decaf->is_con() && !((t + 1) % con_interval))
    {
      double* pos = (double*)decaf->get();
      // debug
//       fprintf(stderr, "num atoms = %d\n", decaf->get_nitems() / 3);
//       for (int i = 0; i < decaf->get_nitems() / 3; i++)
//         fprintf(stderr, "%.3lf %.3lf %.3lf\n", pos[3 * i], pos[3 * i + 1], pos[3 * i + 2]);
    }

    // cleanup at the end of the time step
    decaf->flush(); // both producer and consumer need to clean up after each time step
    // now safe to cleanup producer data, after decaf->flush() is called
    // don't wory about deleting the data pointed to by cd; decaf did that in flush()
    if (decaf->is_prod())
      delete[] x;
  }

  // cleanup
  if (decaf->is_prod())
    delete lammps;
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
