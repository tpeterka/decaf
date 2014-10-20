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
// selects from decaf->data() the subset going to dataflow desination rank dflow_dest
// sets that selection in decaf->data()
// returns number of items selected
// items must be in terms of the elementary datatype defined in decaf->data()
int prod_selector(Decaf* decaf, int dflow_dest)
{
  // this example is simply passing through the original single integer
  // decaf->data()->data_ptr remains unchanged, and the number of datatypes remains 1
  return 1;
}
// user-defined selector code
// runs in the dataflow
// selects from decaf->data() the subset going to dataflow desination rank con_dest
// sets that selection in decaf->data()
// returns number of items selected
// items must be in terms of the elementary datatype defined in decaf->data()
int dflow_selector(Decaf* decaf, int con_dest)
{
  // this example is taking all the original producer ranks and sending to all dataflow ranks
  // decaf->data()->data_ptr remains unchanged, but the number of items is prod_size
  return decaf->sizes()->prod_size;
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
                           &prod_selector,
                           &dflow_selector,
                           &pipeliner,
                           &checker,
                           &data);
  decaf->err();

  // producer
  if (decaf->is_prod())
  {
    int* d = new int[1];
    for (int t = 0; t < tot_time_steps; t++)
    {
      // any custom producer (eg. simulation code) goes here or gets called from here
      // as long as put() gets called at that desired frequency
      *d = t;
      fprintf(stderr, "+ producing time step %d, val %d\n", t, *d);
      // assumes the consumer has the previous value, ok to overwrite
      if (!(t % con_interval))
        decaf->put(d);
    }
    // assumes the consumer has the previous value, ok to delete
    delete[] d;
  }

  // consumer
  if (decaf->is_con())
  {
    for (int t = 0; t < tot_time_steps; t+= con_interval)
    {
      // any custom consumer (eg. data analysis code) goes here or gets called from here
      // as long as get() gets called at that desired frequency and
      // data that decaf allocate are deleted
      int* d = (int*)decaf->get();
      // for example, add all the items arrived at this rank
      int sum = 0;
      for (int i = 0; i < decaf->data()->nitems(); i++)
      {
        fprintf(stderr, "t = %d d[%d] = %d\n", t, i, d[i]);
        sum += d[i];
      }
      fprintf(stderr, "- consuming time step %d, sum = %d\n", t, sum);
      decaf->del();
    }

  }

  // cleanup
  delete decaf;
  MPI_Finalize();
}
