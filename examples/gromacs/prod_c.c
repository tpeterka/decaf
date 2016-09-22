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

#include <decaf/C/cdecaf.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_STRING_LENGTH 512
#define NB_PARTICLES 1024

// producer
void prod(dca_decaf decaf)
{
    // produce data for some number of timesteps
    int timestep;
    float* particles = (float*)(malloc(NB_PARTICLES * 3 * sizeof(float)));
    unsigned int i;
    for(i = 0; i < NB_PARTICLES; i++)
        particles[i] = 1.0f;

    for (timestep = 0; timestep < 10; timestep++)
    {
        fprintf(stderr, "producer timestep %d\n", timestep);

        // the data in this example is just the timestep; add it to a container
        bca_field field = bca_create_arrayfield(particles, bca_FLOAT,
                                                NB_PARTICLES * 3, 3,
                                                NB_PARTICLES * 3, false);

        bca_constructdata container = bca_create_constructdata();

        bca_append_field( container, "pos", field, bca_ZCURVEKEY, bca_PRIVATE,
                          bca_SPLIT_DEFAULT, bca_MERGE_APPEND_VALUES, false);
        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        dca_put(decaf, container);

        bca_free_field(field);
        bca_free_constructdata(container);
    }

    free(particles);

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "producer terminating\n");
    dca_terminate(decaf);
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{

    // Preparing Decaf before the initialization of Gromacs structures
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. "
                "Please export DECAF_PREFIX to point to the root of your decaf "
                "install directory.\n");
        exit(1);
    }

    char libpath[MAX_STRING_LENGTH];
    char path[MAX_STRING_LENGTH];
    strcpy(libpath, "/examples/gromacs/libmod_dflow_gromacs.so");
    strcpy(path, prefix);
    strcat(path, libpath);

    // define the workflow

    MPI_Init(NULL, NULL);

    int in_link = 0;
    int out_link = 0;
    dca_decaf decaf = dca_create_decaf(MPI_COMM_WORLD);
    dca_append_workflow_node(decaf, 0, 2, "gmx", 0, NULL, 1, &out_link);
    dca_append_workflow_node(decaf, 3, 1, "con", 1, &in_link, 0, NULL);
    dca_append_workflow_link(decaf, 0, 1, 2, 1, "dflow", path, "count", "count");

    dca_init_decaf(decaf);

    // run decaf
    prod(decaf);

    // cleanup
    dca_free_decaf( decaf );
    MPI_Finalize();

    return 0;
}
