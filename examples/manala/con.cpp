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
#include <unistd.h>

using namespace decaf;
using namespace std;

// Command flag. 0 = put, other = block
// MPI One-sided stuff
MPI_Win winTest;
int flag = 3;

// Buffering stuff
static MPI_Win winBuffer;
static int send = 0;


// consumer
void con(Decaf* decaf)
{
    map<string, pConstructData> in_data;
    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the value
        SimpleFieldi field = in_data.at("in")->getFieldData<SimpleFieldi >("var");
        if (field)
            sum = field.getData();
        else
            fprintf(stderr, "Error: null pointer in con\n");

        fprintf(stderr, "consumer sum = %d\n", sum);

        // Signaling the dflow to forward the next message
        //decaf->dataflow(0)->signalRootDflowReady();

        // Producer is putting every second
        // Generate an overflow between the dflow and the consumer
        sleep(3);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "consumer terminating\n");
    decaf->terminate();
}


int main(int argc,
         char** argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "buffer.json");

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run decaf
    con(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();

    return 0;
}
