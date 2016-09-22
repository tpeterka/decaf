//---------------------------------------------------------------------------
//
// datflow between tess and dense
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
#include <decaf/data_model/blockfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "wflow.hpp"                                // defines the workflow for this example
#include "block_serialization.hpp"

using namespace decaf;
using namespace std;

// link callback function
extern "C"
{
    // dataflow for now just forwards everything
    void dflow2(void* args,                          // arguments to the callback
                Dataflow* dataflow,                  // dataflow
                pConstructData in_data)              // input data
    {
        fprintf(stderr, "dflow2\n");
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

int main(int argc,
         char** argv)
{
    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished dflow2\n");
    return 0;
}
