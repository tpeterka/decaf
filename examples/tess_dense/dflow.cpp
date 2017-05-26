//---------------------------------------------------------------------------
//
// dataflow between prod and tess and between tess and dense
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
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/blockfield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "block_serialization.hpp"

using namespace decaf;
using namespace std;

// link callback function
extern "C"
{
    // dataflow between prod and tess gets 3 arrays, repackages them into one, and sends it
    void dflow1(void* args,                          // arguments to the callback
                Dataflow* dataflow,                  // dataflow
                pConstructData in_data)              // input data
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
            // debug
            // fprintf(stderr, "%.3f %.3f %.3f\n", xyzpos[3*i], xyzpos[3*i+1], xyzpos[3*i+2]);
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
                              DECAF_POS, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", block,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

        dataflow->put(container, DECAF_LINK);
    }

    // dataflow between tess and dense for now just forwards everything
    void dflow2(void* args,                          // arguments to the callback
                Dataflow* dataflow,                  // dataflow
                pConstructData in_data)              // input data
    {
        dataflow->put(in_data, DECAF_LINK);
    }

} // extern "C"

int main(int argc,
         char** argv)
{
    // define the workflow
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "tess_dense.json");

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // cleanup
    delete decaf;
    MPI_Finalize();

    return 0;
}
