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
void GetArgs(int argc, char **argv, DecafSizes& decaf_sizes, int& tot_time_steps,
             int& con_interval)
{
  assert(argc >= 9);

  decaf_sizes.prod_size   = atoi(argv[1]);
  decaf_sizes.dflow_size  = atoi(argv[2]);
  decaf_sizes.con_size    = atoi(argv[3]);

  decaf_sizes.prod_start  = atoi(argv[4]);
  decaf_sizes.dflow_start = atoi(argv[5]);
  decaf_sizes.con_start   = atoi(argv[6]);

  tot_time_steps          = atoi(argv[7]);
  con_interval            = atoi(argv[8]);
}

int main(int argc, char** argv)
{

  MPI_Init(&argc, &argv);

  // decaf size info
  DecafSizes decaf_sizes;
  int tot_time_steps; // total number of producer time steps
  int con_interval; // consumer is called every so many time steps
  GetArgs(argc, argv, decaf_sizes, tot_time_steps, con_interval);
  decaf_sizes.nsteps = ceil((double)tot_time_steps / con_interval); // consumer time steps

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

  // producer and consumer data
  // keep these in separate pointers in ase producer and consumer overlap
  int *pd, *cd;

  for (int t = 0; t < tot_time_steps; t++)
  {
    // producer
    if (decaf->is_prod())
    {
      pd = new int[1];
      // any custom producer (eg. simulation code) goes here or gets called from here
      // as long as put() gets called at that desired frequency
      *pd = t;
      fprintf(stderr, "+ producing time step %d, val %d\n", t, *pd);
      // assumes the consumer has the previous value, ok to overwrite
      if (!(t % con_interval))
        decaf->put(pd);
    }

    // consumer
    if (decaf->is_con() && !(t % con_interval))
    {
      // any custom consumer (eg. data analysis code) goes here or gets called from here
      // as long as get() gets called at that desired frequency
      cd = (int*)decaf->get();
      // for example, add all the items arrived at this rank
      int sum = 0;
      fprintf(stderr, "consumer get_nitems = %d\n", decaf->data()->get_nitems());
      for (int i = 0; i < decaf->data()->get_nitems(); i++)
        sum += cd[i];
      fprintf(stderr, "- consuming time step %d, sum = %d\n", t, sum);
    }

    decaf->flush(); // both producer and consumer need to clean up after each time step
    // now safe to cleanup producer data, after decaf->flush() is called
    // don't wory about deleting the data pointed to by cd; decaf did that in fluwh()
    if (decaf->is_prod())
      delete[] pd;
  }

  // cleanup
  delete decaf;
  MPI_Finalize();
}
