//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
// this file contains the consumer (2 procs)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "wflow_gromacs.hpp"                         // defines the workflow for this example

using namespace decaf;
using namespace std;

// consumer
void treatment1(Decaf* decaf)
{
    vector< pConstructData > in_data;
    fprintf(stderr, "Launching treatment\n");
    fflush(stderr);
    while (decaf->get(in_data))
    {
        // get the values and add them
        fprintf(stderr, "Reception of %u messages.\n", in_data.size());
        for (size_t i = 0; i < in_data.size(); i++)
        {
            fprintf(stderr, "Number of particles received : %i\n", in_data[i]->getNbItems());
        }
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "Treatment terminating\n");
    decaf->terminate();
}

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                             // workflow
{
    MPI_Init(NULL, NULL);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "treatment rank %i\n", rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);
    fprintf(stderr, "Decaf created\n");
    // start the task
    treatment1(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    fprintf(stderr, "Hello treatment\n");
    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    // run decaf
    run(workflow);

    return 0;
}
