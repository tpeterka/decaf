#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <bredala/transport/mpi/redist_count_mpi.h>

#include "tools.hpp"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>


using namespace decaf;


void runTestCount(int startSource, int nbSources, int startDest, int nbDests)
{
    if(nbSources >= nbDests)
    {
        fprintf(stderr, "ERROR: the number of destination should be larger than the number of sources. Abording\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Checking the size of the communicator
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if((size_world < startSource + nbSources) || (size_world < startDest + nbDests))
    {
        fprintf(stderr,"ERROR: the communicator size is too small. Abording.\n");
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    // Returning the ranks not involved in the redistribution
    if (!isBetween(rank, startSource, nbSources)
     && !isBetween(rank, startDest, nbDests))
        return;

    // Creation of the redistribution component
    RedistCountMPI *component = new RedistCountMPI(startSource,
                                                   nbSources,
                                                   startDest,
                                                   nbDests,
                                                   MPI_COMM_WORLD,
                                                   DECAF_REDIST_COLLECTIVE);

    if (isBetween(rank, startSource, nbSources))
    {
        SimpleFieldi field(1);
        pConstructData container;

        container->appendData(std::string("var"),
                              field,
                              DECAF_NOFLAG,
                              DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT,
                              DECAF_MERGE_DEFAULT);

        component->process(container, DECAF_REDIST_SOURCE);
    }

    if (isBetween(rank, startDest, nbDests))
    {
        pConstructData result;

        component->process(result, DECAF_REDIST_DEST);

        int dest_rank = rank - startDest;
        int nb_items = nbSources;
        if(dest_rank < nb_items)
            fprintf(stderr, "Expecting 1 item.\n");
        else
            fprintf(stderr, "Expecting 0 item.\n");
        result->printKeys();
    }

    return;
}


int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);


    char processorName[MPI_MAX_PROCESSOR_NAME];
    int size_world, rank, nameLen;

    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processorName,&nameLen);

    srand(time(NULL) + rank * size_world + nameLen);

    runTestCount(0, 2, 2, 3);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
