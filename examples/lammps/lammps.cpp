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

#include "lammps.h"
#include "input.h"

using namespace decaf;
using namespace LAMMPS_NS;

// user-defined selector code
// runs in the producer
// selects from decaf->data() the subset going to dataflow
// sets that selection in decaf->data()
// returns number of items selected
// items must be in terms of the elementary datatype defined in decaf->data()
int selector(Decaf* decaf)
{
  // this example is simply passing through the original single integer
  // decaf->data()->data_ptr remains unchanged, and the number of datatypes remains 1
  return 1;
}
// user-defined pipeliner code
void pipeliner(Decaf* decaf)
{
}
// user-defined resilience code
void checker(Decaf* decaf)
{
}
//
// gets command line args
//
void GetArgs(int argc, char **argv, DecafSizes& decaf_sizes, int& prod_nsteps, char* infile)
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

void run(DecafSizes& decaf_sizes, int prod_nsteps, char* infile)
{
  MPI_Init(NULL, NULL);

  // define the data type
  Data data(MPI_INT);

  // start decaf, allocate on the heap instead of on the stack so that it can be deleted
  // before MPI_Finalize is called at the end
  Decaf* decaf = new Decaf(MPI_COMM_WORLD,
                           decaf_sizes,
                           &selector,
                           &pipeliner,
                           &checker,
                           &data);
  decaf->err();

  // run lammps as the producer
  if (decaf->is_prod())
  {
    LAMMPS *lammps = new LAMMPS(0, NULL, decaf->prod_comm());
    lammps->input->file(infile);
    delete lammps;
  }

  // cleanup
  delete decaf;
  MPI_Finalize();
}

int main(int argc, char** argv)
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
