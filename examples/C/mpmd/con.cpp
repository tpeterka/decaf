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

// consumer
void con(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data[i]->getFieldData<SimpleFieldi>("var");
            if (field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in con\n");
        }
        fprintf(stderr, "consumer sum = %d\n", sum);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "consumer terminating\n");
    decaf->terminate();
}

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                             // workflow
{
    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // start the task
    con(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    // run decaf
    run(workflow);

    return 0;
}
