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
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/blockfield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

using namespace decaf;
using namespace std;


// producer
void prod(Decaf* decaf, int tot_npts, int nsteps)
{
    // hacc produces 3 arrays of position coordinates
    // we're making up some synthetic data instead

    // number of points, randomly placed in a box of size [0, npts - 1]^3
    int npts = tot_npts / decaf->local_comm_size();
    fprintf(stderr, "Generates locally %i particles.\n", npts);

    // add 1 to the rank because on some compilers, srand(0) = srand(1)
    srand(decaf->world->rank() + 1);

    // produce data for some number of timesteps
    for (int timestep = 0; timestep < nsteps; timestep++)
    {
        // create the synthetic points for the timestep
        fprintf(stderr, "producer timestep %d\n", timestep);

        // the point arrays need to be allocated inside the loop because
        // decaf will automatically free them at the end of the loop after sending
        // (smart pointer reference counting)
        float* x = new float[npts];
        float* y = new float[npts];
        float* z = new float[npts];
        for (unsigned i = 0; i < npts; ++i)
        {
            // debug: test what happens when a point is duplicated or outside the domain
            // if (i == 1)                           // duplicate point test
            // {
            //     x[i] = x[i - 1];
            //     y[i] = y[i - 1];
            //     z[i] = z[i - 1];
            // }
            // else if (i == npts  - 1)              // out of bounds test
            // {
            //     float t = (float) rand() / RAND_MAX;
            //     x[i] = npts;
            //     t = (float) rand() / RAND_MAX;
            //     y[i] = t * (npts - 1);
            //     t = (float) rand() / RAND_MAX;
            //     z[i] = t * (npts - 1);
            // }
            // else
            // {
                float t = (float) rand() / RAND_MAX;
                x[i] = t * (npts - 1);
                t = (float) rand() / RAND_MAX;
                y[i] = t * (npts - 1);
                t = (float) rand() / RAND_MAX;
                z[i] = t * (npts - 1);
            // }
        }

        // mins and sizes, not mins and maxs
        vector<unsigned int> extendsBlock = {0, 0, 0,
                                             (unsigned)npts, (unsigned)npts, (unsigned)npts};
        vector<float> bboxBlock = {0.0, 0.0, 0.0, (float)npts, float(npts), float(npts)};

        // describe to decaf the global and local domain
        BlockField domainData(true);
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
    MPI_Init(NULL, NULL);

    if(argc < 3)
    {
        fprintf(stderr, "Usage: points tot_npts nsteps\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    int tot_npts = atoi(argv[1]);
    int nsteps   = atoi(argv[2]);
    fprintf(stderr, "Will generate %i particles in total.\n", tot_npts);

    // define the workflow
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "tess_dense.json");

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // timing
    MPI_Barrier(decaf->prod_comm_handle());
    double t0 = MPI_Wtime();
    if (decaf->prod_comm()->rank() == 0)
        fprintf(stderr, "*** workflow starting time = %.3lf s. ***\n", t0);

    // start the task
    prod(decaf, tot_npts, nsteps);

    // timing
    MPI_Barrier(decaf->prod_comm_handle());
    if (decaf->prod_comm()->rank() == 0)
        fprintf(stderr, "*** prod. elapsed time = %.3lf s. ***\n", MPI_Wtime() - t0);

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished prod\n");
    return 0;
}
