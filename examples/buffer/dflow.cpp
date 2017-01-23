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
#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/boost_macros.h>

#include "buffer.h"

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
static int* arrayIt = NULL;

// Buffering stuff
MPI_Win winBuffer;          // Declared in buffer.h Necessary as the object is used in the library call
static int windowSend = 0;
static int localSend = 0;

static std::stack<pConstructData> queue;

// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        fprintf(stderr, "Forwarding data in dflow\n");
        fprintf(stderr, "Iteration %i\n", iteration);
        if(iteration == 0)
        {
            arrayIt = new int[1];
            arrayIt[0] = 0;
        }

        fprintf(stderr, "Iteration array %i\n", arrayIt[0]);
        iteration++;
        arrayIt[0] = arrayIt[0] + 1;

        // Wait that we receive the signal to send the next message
        do
        {
            // Exclusive access so the consumer can not change the value
            // during the check.
            // TODO: atomic read?
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 1, 0, winBuffer);
            MPI_Get(&localSend, 1, MPI_INT, 1, 0, 1, MPI_INT, winBuffer);
            if(localSend == 0) // We are free to send. Removing the flag
            {
                int newFlag = 1;
                MPI_Put(&newFlag, 1, MPI_INT, 1, 0, 1, MPI_INT, winBuffer);
            }
            MPI_Win_unlock(1, winBuffer);
        } while (localSend != 0);

        dataflow->put(in_data, DECAF_LINK);
        localSend = 1;
    }
} // extern "C"

int main(int argc,
         char** argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "buffer.json");

    MPI_Init(NULL, NULL);

    //Initialization of the window for buffering
    MPI_Win_create(&windowSend, 1, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &winBuffer);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    MPI_Win_free(&winBuffer);

    // cleanup
    delete decaf;
    MPI_Finalize();

    return 0;
}
