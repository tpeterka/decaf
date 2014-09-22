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

// user-defined consumer code
void con_code(void* args)
{
}
// user-defined selector code
void sel_type(void *args)
{
}
// user-defined pipeliner code
void pipe_type(void *args)
{
}
// user-defined aggregator code
void aggr_type(void *args)
{
}
// user-defined resilience code
void fault_check(void *args)
{
}

int main(int argc, char** argv)
{

  MPI_Init(&argc, &argv);

  // create (split) new communicators
  int prod_size = 4;  // fake some communicator sizes: producer
  int con_size = 2;   // consumer
  int dflow_size = 1; // dataflow size defines number of aggregator and other intermediate nodes
  int tot_time_steps = 10; // total time steps
  int con_interval = 2; // consumer frequency (in time steps)

  // start decaf, allocate on the heap instead of on the stack so that it can be deleted
  // before MPI_Finalize is called at the end
  decaf::Decaf* decaf = new decaf::Decaf(MPI_COMM_WORLD, prod_size, con_size, dflow_size);
  decaf->err();
  decaf::Data data(MPI_INT);

  // producer
  if (decaf->type() == DECAF_PRODUCER_COMM)
  {
    decaf::Producer prod(decaf->out_comm(), data);

    // produce
    int* d = new int[1];
    for (int t = 0; t < tot_time_steps; t++)
    {
      // any custom producer (eg. simulation code) goes here or gets called from here
      // as long as prod.put() gets called at that desired frequency
      *d = t;
      fprintf(stderr, "+ producing time step %d, val %d\n", t, *d);
      // assumes the consumer has the previous value, ok to overwrite
      if (!(t % con_interval))
        prod.put(d);
    }
    // assumes the consumer has the previous value, ok to delete
    delete[] d;
  }

  // consumer
  else if (decaf->type() == DECAF_CONSUMER_COMM)
  {
    decaf::Consumer con(decaf->in_comm(),
                        &con_code,
                        &sel_type,
                        &pipe_type,
                        &aggr_type,
                        &fault_check,
                        data);
    // consume
    for (int t = 0; t < tot_time_steps; t+= con_interval)
    {
      // any custom consumer (eg. data analysis code) goes here or gets called from here
      // as long as con.get() gets called at that desired frequency
      int* d = (int*)con.get();
      fprintf(stderr, "- consuming time step %d, val %d\n", t, *d);
      con.del_data();
    }

  }

  // dataflow
  else if (decaf->type() == DECAF_DATAFLOW_COMM)
    // TODO: computing nsteps is awkward
    decaf::Dataflow dflow(decaf->out_comm(), decaf->in_comm(), data,
                          ceil((double)tot_time_steps / con_interval));

  // cleanup
  delete decaf;
  MPI_Finalize();
}
