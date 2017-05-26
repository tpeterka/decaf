//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
// this file contains the producer (4 procs)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "wflow.hpp"                         // defines the workflow for this example

using namespace decaf;
using namespace std;

// producer
void prod(Decaf* decaf)
{
    // produce data for some number of timesteps
    for (int timestep = 0; timestep < 10; timestep++)
    {
        fprintf(stderr, "producer timestep %d\n", timestep);

        // the data in this example is just the timestep; add it to a container
        SimpleFieldi data(timestep);

        pConstructData container;
        container->appendData("var", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "producer terminating\n");
    decaf->terminate();
}

int main(int argc,
         char** argv)
{
    // define the workflow
    Workflow workflow;
    //make_wflow(workflow);
    Workflow::make_wflow_from_json(workflow, "linear2.json");

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // start the task
    prod(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();

    return 0;
}
