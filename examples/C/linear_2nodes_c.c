#include <decaf/C/cdecaf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_LENGTH 512

// producer
void prod(dca_decaf decaf)
{
    // produce data for some number of timesteps
    int timestep;
    for (timestep = 0; timestep < 10; timestep++)
    {
        fprintf(stderr, "producer timestep %d\n", timestep);

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
    fprintf(stderr, "producer terminating\n");
    dca_terminate(decaf);
}

// consumer
void con(dca_decaf decaf)
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
        fprintf(stderr, "consumer sum = %d\n", sum);
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

    int out_link = 0;
    dca_append_workflow_node(decaf, 0, 4, "prod", 0, NULL, 1, &out_link);

    int in_link = 0;
    dca_append_workflow_node(decaf, 6, 2, "con", 1, &in_link, 0, NULL);

    dca_append_workflow_link(decaf, 0, 1, 4, 2, "dflow",
                             path, "count", "count");
    MPI_Init(NULL, NULL);

    dca_init_decaf(decaf);

    if(dca_my_node(decaf, "prod"))
        prod(decaf);
    if(dca_my_node(decaf, "con"))
        con(decaf);

    dca_free_decaf(decaf);
    MPI_Finalize();

    return 0;
}
