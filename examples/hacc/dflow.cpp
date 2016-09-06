//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
// this file contains the dataflow (2 procs) and the global workflow, run function
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

using namespace decaf;
using namespace std;

// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        ArrayFieldf xposArray = in_data->getFieldData<ArrayFieldf>("x_pos");
        if(!xposArray)
        {
            fprintf(stderr, "ERROR dflow: unable to find the field required \"x_pos\" "
                    "in the data model.\n");
            return;
        }
        ArrayFieldf yposArray = in_data->getFieldData<ArrayFieldf>("y_pos");
        if(!yposArray)
        {
            fprintf(stderr, "ERROR dflow: unable to find the field required \"y_pos\" "
                    "in the data model.\n");
            return;
        }
        ArrayFieldf zposArray = in_data->getFieldData<ArrayFieldf>("z_pos");
        if(!zposArray)
        {
            fprintf(stderr, "ERROR dflow: unable to find the field required \"z_pos\" "
                    "in the data model.\n");
            return;
        }


        float* xpos = xposArray.getArray();
        float* ypos = yposArray.getArray();
        float* zpos = zposArray.getArray();
        assert (xposArray->getNbItems() == yposArray->getNbItems() &&
                xposArray->getNbItems() == zposArray->getNbItems());
        int nbParticle = xposArray->getNbItems();

        float* xyzpos = new float[nbParticle *3];
        for (int i = 0; i < nbParticle; i++)
        {
            xyzpos[3 * i    ] = xpos[i];
            xyzpos[3 * i + 1] = ypos[i];
            xyzpos[3 * i + 2] = zpos[i];
        }


        BlockField block = in_data->getFieldData<BlockField>("domain_block");
        if(!block)
        {
            fprintf(stderr, "ERROR dflow: unable to find the field required \"domain_block\" "
                    "in the data model.\n");
            return;
        }

        pConstructData container;
        ArrayFieldf xyz_pos(xyzpos, 3 * nbParticle, 3, true); // true = decaf will free xyzpos
        container->appendData(string("xyz_pos"), xyz_pos,
                              DECAF_ZCURVEKEY, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", block,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

        dataflow->put(container, DECAF_LINK);
    }
} // extern "C"

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)
{
    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

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
