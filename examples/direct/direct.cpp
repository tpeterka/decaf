//---------------------------------------------------------------------------
//
// example of direct coupling
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

using namespace decaf;

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
             int& prod_nsteps)
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
}

void run(DecafSizes& decaf_sizes,
         int prod_nsteps)
{
  MPI_Init(NULL, NULL);

  // define the data type
  Data data(MPI_INT);

  // start decaf, allocate on the heap instead of on the stack so that it can be deleted
  // before MPI_Finalize is called at the end
  Decaf* decaf = new Decaf(MPI_COMM_WORLD,
                           decaf_sizes,
                           &pipeliner,
                           &checker,
                           &data);
  decaf->err();
  decaf->run();

  // producer and consumer data in separate pointers in case producer and consumer overlap
  int *pd, *cd;
  int con_interval = prod_nsteps / decaf_sizes.con_nsteps; // consume every so often

  for (int t = 0; t < prod_nsteps; t++)
  {
    // producer
    if (decaf->is_prod())
    {
      pd = new int[1];
      // any custom producer (eg. simulation code) goes here or gets called from here
      // as long as put() gets called at that desired frequency
      *pd = t;
      // assumes the consumer has the previous value, ok to overwrite
      // check your modulo arithmetic to ensure you put exactly decaf->con_nsteps times
      if (!((t + 1) % con_interval))
      {
        fprintf(stderr, "+ producing time step %d, val %d\n", t, *pd);
        decaf->put(pd); // TODO: dataflow not handling different tags yet
      }
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly decaf->con_nsteps times
    if (decaf->is_con() && !((t + 1) % con_interval))
    {
      // any custom consumer (eg. data analysis code) goes here or gets called from here
      // as long as get() gets called at that desired frequency
      cd = (int*)decaf->get(); // TODO: dataflow not handling different tags yet
      // for example, add all the items arrived at this rank
      int sum = 0;
//       fprintf(stderr, "consumer get_nitems = %d\n", decaf->get_nitems(true));
      for (int i = 0; i < decaf->get_nitems(); i++)
        sum += cd[i];
      fprintf(stderr, "- consuming time step %d, sum = %d\n", t, sum);
    }

    decaf->flush(); // both producer and consumer need to clean up after each time step
    // now safe to cleanup producer data, after decaf->flush() is called
    // don't wory about deleting the data pointed to by cd; decaf did that in flush()
    if (decaf->is_prod())
      delete[] pd;
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
  GetArgs(argc, argv, decaf_sizes, prod_nsteps);

  // run decaf
  run(decaf_sizes, prod_nsteps);

  return 0;
}
