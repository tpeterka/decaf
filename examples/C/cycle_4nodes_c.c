//---------------------------------------------------------------------------
//
// 4-node graph with 2 cycles
//
//       <---------  node_b (1 proc, rank 5)
//      |                                  |
//     dflow (1 proc, rank 10)             |
//      |                                  |
//      |        ->dflow (1 proc, rank 4) ->
//      |       |
//      -> node_a (4 procs, ranks 0-3)
//         |
//         dflow (1 proc, rank 6)
//         |
//         -> node_c (1 proc, rank 7) --> dflow (1 proc, rank 8) --> node_d (1 proc, rank9)
//
//  entire workflow takes 11 procs (1 dataflow proc between each producer consumer pair)
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

void node_a(dca_decaf decaf)
{
    // produce data for some number of timesteps
    int timestep;
    for (timestep = 0; timestep < 10; timestep++)
    {
        int sum = 0;

        if (timestep > 0)
        {
            // receive data from all inbound dataflows
            // in this example there is only one inbound dataflow,
            // but in general there could be more
            bca_constructdata* in_data = NULL;
            unsigned int nb_data = 0;
            in_data = dca_get(decaf, &nb_data);

            // get the values and add them
            unsigned int i;
            for (i = 0; i < nb_data; i++)
            {
                bca_field field = bca_get_simplefield(in_data[i], "vars", bca_INT);
                if(field)
                {
                    int value;
                    bca_get_data(field, bca_INT, &value);
                    sum += value;
                }

                bca_free_field(field);
                bca_free_constructdata(in_data[i]);
            }

            free(in_data);
        }

        fprintf(stderr, "node_a: timestep %d sum = %d\n", timestep, sum);

        // the data in this example is just the timestep, add it to a container
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
    fprintf(stderr, "node_a terminating\n");
    dca_terminate(decaf);
}

void node_b(dca_decaf decaf)
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

        fprintf(stderr, "node_b: sum = %d\n", sum);

        // append the array to a container
        bca_field field = bca_create_simplefield(&sum, bca_INT);
        bca_constructdata container = bca_create_constructdata();
        bca_append_field(container, "vars", field,
                         bca_NOFLAG, bca_PRIVATE,
                         bca_SPLIT_DEFAULT, bca_MERGE_APPEND_VALUES);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        dca_put(decaf, container);

        bca_free_field(field);
        bca_free_constructdata(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_b terminating\n");
    dca_terminate(decaf);
}

void node_c(dca_decaf decaf)
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

        fprintf(stderr, "node_c: sum = %d\n", sum);

        // append the sum to a container
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
    fprintf(stderr, "node_c terminating\n");
    dca_terminate(decaf);
}

void node_d(dca_decaf decaf)
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

        fprintf(stderr, "node_d: sum = %d\n", sum);

        // add 1 to the sum and send it back (for example)
        sum++;
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
    fprintf(stderr, "node_d terminating\n");
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

    MPI_Init(NULL,NULL);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stdout, "My rank : %i\n", rank);

    dca_decaf decaf = dca_create_decaf(MPI_COMM_WORLD);


    // fill workflow nodes
    int in_link = 1;
    int out_link = 3;
    dca_append_workflow_node(decaf, 5, 1, "node_b", 1, &in_link, 1, &out_link);

    in_link = 0;
    dca_append_workflow_node(decaf, 9, 1, "node_c", 1, &in_link, 0, NULL);

    in_link = 2;
    out_link = 0;
    dca_append_workflow_node(decaf, 7, 1, "node_c", 1, &in_link, 1, &out_link);

    int out_links[2];
    out_links[0] = 1;
    out_links[1] = 2;
    in_link = 3;
    dca_append_workflow_node(decaf, 0, 4, "node_a", 1, &in_link, 2, out_links);

    // fill workflow links

    // node_c -> node_d
    dca_append_workflow_link(decaf, 2, 1, 8, 1, "dflow",
                             path, "count", "count");

    // node_a -> node_b
    dca_append_workflow_link(decaf, 3, 0, 4, 1, "dflow",
                             path, "count", "count");

    // node_a -> node_c
    dca_append_workflow_link(decaf, 3, 2, 6, 1, "dflow",
                             path, "count", "count");

    // node_b -> node_a
    dca_append_workflow_link(decaf, 0, 3, 10, 1, "dflow",
                             path, "count", "count");

    dca_init_decaf(decaf);

    if (dca_my_node(decaf, "node_a"))
        node_a(decaf);
    if (dca_my_node(decaf, "node_b"))
        node_b(decaf);
    if (dca_my_node(decaf, "node_c"))
        node_c(decaf);
    if (dca_my_node(decaf, "node_d"))
        node_d(decaf);

    // identify sources
    //vector<int> sources;
    //sources.push_back(3);                           // node_a

    dca_free_decaf(decaf);
    MPI_Finalize();

    return 0;
}
