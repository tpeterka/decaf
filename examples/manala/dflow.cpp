//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example with buffering mechanism
//
// prod (1 procs) - con (1 procs)
//
// entire workflow takes 3 procs (1 dataflow procs between prod and con)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <stack>

using namespace decaf;
using namespace std;


// Everything static because dflow is a library call
static int iteration = 0;

// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        fprintf(stderr, "It %i: Forwarding data in dflow\n", iteration);
        iteration++;

        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

int main(int argc,
         char** argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "buffer.json");

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // cleanup
    delete decaf;
    MPI_Finalize();

    return 0;
}
