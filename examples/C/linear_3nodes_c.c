//---------------------------------------------------------------------------
//
// 3-node linear coupling example
//
// node0 (4 procs) - node1 (2 procs) - node2 (1 proc)
// node0[0,3] -> dflow[4,6] -> node1[7,8] -> dflow[9,10] -> node2[11]
//
// entire workflow takes 12 procs (3 dataflow procs between node0 and node1 and
// 2 dataflow procs between node1 and node2)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/C/cdecaf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_LENGTH 512

// producer
void node0(dca_decaf decaf)
{
    // produce data for some number of timesteps
    int timestep;
    for (timestep = 0; timestep < 10; timestep++)
    {
        fprintf(stderr, "node0 timestep %d\n", timestep);

        // the data in this example is just the timestep; add it to a container
        bca_field field =  bca_create_simplefield(&timestep, bca_INT);
        bca_constructdata container = bca_create_constructdata();

        if(!bca_append_field(container, "var", field,
                             bca_NOFLAG, bca_PRIVATE,
                             bca_SPLIT_KEEP_VALUE, bca_MERGE_ADD_VALUE))
        {
            fprintf(stderr, "ERROR : unable to append the field \"var\" in the container\n");
        }

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        dca_put(decaf, container);

        bca_free_field(field);
        bca_free_constructdata(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node0 terminating\n");
    dca_terminate(decaf);
}

// intermediate node is both a consumer and producer
void node1(dca_decaf decaf)
{
    bca_constructdata* in_data = NULL;
    unsigned int nb_data = 0;

    while ((in_data = dca_get(decaf, &nb_data)) != NULL)
    {
        int sum = 0;

        // get the values and add them
        size_t i;
        for (i = 0; i < nb_data; i++)
        {
            bca_field field = bca_get_simplefield(
                        in_data[i], "var", bca_INT);
            if(field)
            {
                int value;
                bca_get_data(field, bca_INT, &value);
                sum += value;
            }
            else
                fprintf(stderr, "Error: null pointer in con\n");

            bca_free_field(field);
            bca_free_constructdata(in_data[i]);

        }
        free(in_data);
        fprintf(stderr, "node1 sum = %d\n", sum);

        // the data in this example is just the timestep; add it to a container
        bca_field field =  bca_create_simplefield(&sum, bca_INT);
        bca_constructdata container = bca_create_constructdata();

        if(!bca_append_field(container, "var", field,
                             bca_NOFLAG, bca_PRIVATE,
                             bca_SPLIT_KEEP_VALUE, bca_MERGE_ADD_VALUE))
        {
            fprintf(stderr, "ERROR : unable to append the field \"var\" in the container\n");
        }

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        dca_put(decaf, container);

        bca_free_field(field);
        bca_free_constructdata(container);

    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node1 terminating\n");
    dca_terminate(decaf);
}

// consumer
void node2(dca_decaf decaf)
{
    bca_constructdata* in_data = NULL;
    unsigned int nb_data = 0;

    while ((in_data = dca_get(decaf, &nb_data)) != NULL)
    {
        int sum = 0;

        // get the values and add them
        size_t i;
        for (i = 0; i < nb_data; i++)
        {
            bca_field field = bca_get_simplefield(
                        in_data[i], "var", bca_INT);
            if(field)
            {
                int value;
                bca_get_data(field, bca_INT, &value);
                sum += value;
            }
            else
                fprintf(stderr, "Error: null pointer in con\n");

            // We need to each each bca object
            bca_free_field(field);
            bca_free_constructdata(in_data[i]);
        }
        fprintf(stderr, "node2 sum = %d\n", sum);
        free(in_data);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "consumer terminating\n");
    dca_terminate(decaf);
}

int main()
{
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. "
                "Please export DECAF_PREFIX to point to the root of your decaf "
                "install directory.\n");
        exit(1);
    }

    char suffix[MAX_STRING_LENGTH];
    char path[MAX_STRING_LENGTH];
    strcpy(suffix, "/examples/C/decaf/libmod_dflow_direct.so");
    strcpy(path, prefix);
    strcat(path, suffix);

    dca_decaf decaf = dca_create_decaf(MPI_COMM_WORLD);

    // fill workflow nodes
    // node0
    int out_link = 0;
    dca_append_workflow_node(decaf, 0, 4, "node0", 0, NULL, 1, &out_link);

    // node1
    int in_link = 0;
    out_link = 1;
    dca_append_workflow_node(decaf, 7, 2, "node1", 1, &in_link, 1, &out_link);

    // node2
    in_link = 1;
    dca_append_workflow_node(decaf, 11, 1, "node2", 1, &in_link, 0, NULL);

    // fill workflow links
    // dataflow between node0 and node1
    dca_append_workflow_link(decaf, 0, 1, 4, 3, "dflow",
                             path, "count", "count");

    // dataflow between node1 and node2
    dca_append_workflow_link(decaf, 1, 2, 9, 2, "dflow",
                             path, "count", "count");

    MPI_Init(NULL, NULL);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("My rank : %i\n", rank);

    dca_init_decaf(decaf);

    if(dca_my_node(decaf, "node0"))
        node0(decaf);
    if(dca_my_node(decaf, "node1"))
        node1(decaf);
    if(dca_my_node(decaf, "node2"))
        node2(decaf);

    dca_free_decaf(decaf);
    MPI_Finalize();

    return 0;
}
