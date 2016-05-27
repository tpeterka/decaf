#include <mpi.h>
#include <decaf/C/breadala.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 10

bool isBetween(int rank, int start, int nb)
{
    return rank >= start && rank < start + nb;
}


void runParallelRedist(int rank_source, int nb_sources,
                       int rank_dest, int nb_dests)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stdout, "Rank : %i\n", rank);

    bca_redist component = bca_create_redist(
                bca_REDIST_COUNT,
                rank_source, nb_sources,
                rank_dest, nb_dests,
                MPI_COMM_WORLD);

    if(isBetween(rank, rank_source, nb_sources))
    {
        //Creation of the data model
        int* array = (int*)(malloc(SIZE * sizeof(int)));
        unsigned int i;

        fprintf(stdout, "Initial array [ ");
        for(i = 0; i < SIZE; i++)
        {
            array[i] = rank * SIZE + i;
            fprintf(stdout, "%i ", array[i]);
        }
        fprintf(stdout, "]\n");


        bca_constructdata container = bca_create_constructdata();
        bca_field field_array = bca_create_arrayfield(array, bca_INT, SIZE, 1, SIZE, false);

        if(!bca_append_field(container, "array", field_array,
                             bca_NOFLAG, bca_PRIVATE,
                             bca_SPLIT_DEFAULT, bca_MERGE_DEFAULT))
        {
            fprintf(stderr, "Abord. Unable to append the array in the container\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }


        bca_process_redist(container, component, bca_REDIST_SOURCE);
        bca_flush_redist(component); // Assume no overlapping!

        bca_free_field(field_array);
        bca_free_constructdata(container);
        free(array);
    }

    if(isBetween(rank, rank_dest, nb_dests))
    {
        bca_constructdata container = bca_create_constructdata();
        bca_process_redist(container, component, bca_REDIST_DEST);

        bca_field field_array = bca_get_arrayfield(container, "array", bca_INT);
        if(field_array == NULL)
        {
            fprintf(stderr, "Abord. Unable to request the field array in the container\n");
            MPI_Abort(MPI_COMM_WORLD, 3);
        }

        size_t size;
        int* array = bca_get_array(field_array, bca_INT, &size);
        if(array == NULL)
        {
            fprintf(stderr, "Abord. Unable to extract the array from the field\n");
            MPI_Abort(MPI_COMM_WORLD, 4);
        }

        fprintf(stdout, "Sub array [ ");
        unsigned int i;
        for(i = 0; i < size; i++)
        {
            fprintf(stdout, "%i ", array[i]);
        }
        fprintf(stdout, "]\n");

        bca_flush_redist(component); // Assume no overlapping!

        bca_free_field(field_array);
        bca_free_constructdata(container);
    }

    bca_free_redist(component);
}

int main()
{
    MPI_Init(NULL, NULL);

    int size_world;

    MPI_Comm_size(MPI_COMM_WORLD, &size_world);


    if(size_world < 5)
    {
        fprintf(stderr, "Abord. Require at least 5 MPI processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    runParallelRedist(0,3,3,2);

    MPI_Finalize();
}
