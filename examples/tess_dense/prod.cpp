//---------------------------------------------------------------------------
//
// generates synthetic particles and sends them to the rest of the workflow
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
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

#include "wflow.hpp"                         // defines the workflow for this example

using namespace decaf;
using namespace std;

// producer
void prod(Decaf* decaf)
{
    // hacc produces 3 arrays of position coordinates
    // we're making up some synthetic data instead

    // number of points, randomly placed in a box of size [0, npts - 1]^3
    int npts = 10;
    float* x = new float[npts];
    float* y = new float[npts];
    float* z = new float[npts];

    srand(decaf->world->rank());
    for (unsigned i = 0; i < npts; ++i)
    {
        float t = (float) rand() / RAND_MAX;
        x[i] = t * (npts - 1);
        t = (float) rand() / RAND_MAX;
        y[i] = t * (npts - 1);
        t = (float) rand() / RAND_MAX;
        z[i] = t * (npts - 1);
    }

    // produce data for some number of timesteps
    for (int timestep = 0; timestep < 1; timestep++)
    {
        fprintf(stderr, "producer timestep %d\n", timestep);

        // mins and sizes, not mins and maxs
        vector<unsigned int> extendsBlock = {0, 0, 0,
                                             (unsigned)npts, (unsigned)npts, (unsigned)npts};
        vector<float> bboxBlock = {0.0, 0.0, 0.0, (float)npts, float(npts), float(npts)};

        // describe to decaf the global and local domain
        BlockField domainData;
        Block<3>* domainBlock = domainData.getBlock();
        domainBlock->setGridspace(1.0);
        domainBlock->setGlobalExtends(extendsBlock);
        domainBlock->setLocalExtends(extendsBlock);
        domainBlock->setOwnExtends(extendsBlock);
        domainBlock->setGlobalBBox(bboxBlock);
        domainBlock->setLocalBBox(bboxBlock);
        domainBlock->setOwnBBox(bboxBlock);
        domainBlock->ghostSize_ = 0;

        // define the fields of the data model
        ArrayFieldf x_pos(x, npts, 1, true);
        ArrayFieldf y_pos(y, npts, 1, true);
        ArrayFieldf z_pos(z, npts, 1, true);

        // debug
        // for (int i = 0; i < npts; i++)
        //     fprintf(stderr, "%.3f %.3f %.3f\n", x[i], y[i], z[i]);

        // push the fields into a container
        pConstructData container;
        container->appendData(string("x_pos"), x_pos,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData(string("y_pos"), y_pos,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData(string("z_pos"), z_pos,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", domainData,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

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
    make_wflow(workflow);

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // start the task
    prod(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished prod\n");
    return 0;
}
