#include <mpi.h>
#include <decaf/C/breadala.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 10
#define NBPARTICLES 10

bool isBetween(int rank, int start, int nb)
{
    return rank >= start && rank < start + nb;
}

void initPosition(float* pos, int nbParticule, float* bbox)
{
    float dx = bbox[3] - bbox[0];
    float dy = bbox[4] - bbox[1];
    float dz = bbox[5] - bbox[2];

    for(int i = 0;i < nbParticule; i++)
    {
        pos[3*i] = (bbox[0] + ((float)rand() / (float)RAND_MAX) * dx);
        pos[3*i+1] = (bbox[1] + ((float)rand() / (float)RAND_MAX) * dy);
        pos[3*i+2] = (bbox[2] + ((float)rand() / (float)RAND_MAX) * dz);
    }
}

void initBlock(bca_block* block)
{
    block->ghostsize = 0;
    block->gridspace = 0;
    block->globalbbox = NULL;
    block->globalextends = NULL;
    block->localbbox = NULL;
    block->localextends = NULL;
    block->ownbbox = NULL;
    block->ownextends = NULL;
}

void runParallelRedistBlock(int rank_source, int nb_sources,
                       int rank_dest, int nb_dests)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stdout, "Rank : %i\n", rank);

    bca_redist component = bca_create_redist(
                bca_REDIST_BLOCK,
                rank_source, nb_sources,
                rank_dest, nb_dests,
                MPI_COMM_WORLD);

    if(isBetween(rank, rank_source, nb_sources))
    {
        //Creation of the data model
        float global_bbox[6];
        unsigned int global_extends[6];

        global_bbox[0] = 0.0f;global_bbox[1] = 0.0f;global_bbox[2] = 0.0f;
        global_bbox[3] = 10.0f;global_bbox[4] = 10.0f;global_bbox[5] = 10.0f;
        global_extends[0] = 0;global_extends[1] = 0;global_extends[2] = 0;
        global_extends[3] = 10;global_extends[4] = 10;global_extends[5] = 10;

        bca_block block;
        initBlock(&block);
        block.gridspace = 1.0f;
        block.globalbbox = global_bbox;
        block.globalextends = global_extends;

        float* pos = (float*)(malloc(NBPARTICLES * 3 * sizeof(float)));
        initPosition(pos, NBPARTICLES, global_bbox);


        bca_constructdata container = bca_create_constructdata();
        bca_field field_array = bca_create_arrayfield(pos, bca_FLOAT, SIZE, 1, SIZE, false);
        bca_field field_block = bca_create_blockfield(&block);

        if(!bca_append_field(container, "pos", field_array,
                             bca_ZCURVEKEY, bca_PRIVATE,
                             bca_SPLIT_DEFAULT, bca_MERGE_DEFAULT))
        {
            fprintf(stderr, "Abord. Unable to append the array in the container\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }

        // Convention name to respect
        if(!bca_append_field(container, "domain_block", field_block,
                             bca_ZCURVEKEY, bca_PRIVATE,
                             bca_SPLIT_KEEP_VALUE, bca_MERGE_FIRST_VALUE))
        {
            fprintf(stderr, "Abord. Unable to append the block in the container\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }


        bca_process_redist(container, component, bca_REDIST_SOURCE);
        bca_flush_redist(component); // Assume no overlapping!

        bca_free_field(field_array);
        bca_free_field(field_block);
        bca_free_constructdata(container);
        free(pos);
    }

    if(isBetween(rank, rank_dest, nb_dests))
    {
        bca_constructdata container = bca_create_constructdata();
        bca_process_redist(container, component, bca_REDIST_DEST);

        bca_field field_array = bca_get_arrayfield(container, "pos", bca_FLOAT);
        if(field_array == NULL)
        {
            fprintf(stderr, "Abord. Unable to request the field array in the container\n");
            MPI_Abort(MPI_COMM_WORLD, 3);
        }

        size_t size;
        float* array = bca_get_array(field_array, bca_FLOAT, &size);
        if(array == NULL)
        {
            fprintf(stderr, "Abord. Unable to extract the array from the field\n");
            MPI_Abort(MPI_COMM_WORLD, 4);
        }

        fprintf(stdout, "Sub array [ ");
        unsigned int i;
        for(i = 0; i < size; i++)
        {
            fprintf(stdout, "%f ", array[i]);
        }
        fprintf(stdout, "]\n");

        bca_flush_redist(component); // Assume no overlapping!

        bca_free_field(field_array);
        bca_free_constructdata(container);
    }

    bca_free_redist(component);
}


void runParallelRedistCount(int rank_source, int nb_sources,
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

    char processorName[MPI_MAX_PROCESSOR_NAME];
    int size_world, rank, nameLen;

    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processorName,&nameLen);

    srand(time(NULL) + rank * size_world + nameLen);


    if(size_world < 5)
    {
        fprintf(stderr, "Abord. Require at least 5 MPI processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    runParallelRedistCount(0,3,3,2);
    runParallelRedistBlock(0,3,3,2);

    MPI_Finalize();
}
