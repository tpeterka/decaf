//---------------------------------------------------------------------------
//
// Example of redistribution with Boost serialization and 2 Redistributions
// (1 producer and 2 consumers)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/boost_macros.h>

#include <bredala/transport/mpi/redist_round_mpi.h>
#include "tools.hpp"


using namespace decaf;
using namespace std;

void runTestParallel2RedistOverlap(int startSource, int nbSource,
                                   int startReceptors1, int nbReceptors1,
                                   unsigned long long nbParticle)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!isBetween(rank, startSource, nbSource)
            && !isBetween(rank, startReceptors1, nbReceptors1))
        return;

    RedistRoundMPI *component1 = nullptr;
    unsigned int* buffer = nullptr;

    std::cout<<"Current rank : "<<rank<<std::endl;

    if (isBetween(rank, startSource, nbSource)
            || isBetween(rank, startReceptors1, nbReceptors1))
    {
        std::cout <<"consutrcution for the first component ."<<std::endl;
        component1 = new RedistRoundMPI(startSource,
                                        nbSource,
                                        startReceptors1,
                                        nbReceptors1,
                                        MPI_COMM_WORLD,
                                        DECAF_REDIST_COLLECTIVE);
    }


    fprintf(stderr, "-------------------------------------\n"
            "Test with Redistribution component with overlapping...\n"
            "-------------------------------------\n");


    if (isBetween(rank, startSource, nbSource))
    {
        fprintf(stderr, "Running Redistribution test between %d producers and %d consumers\n",
                nbSource, nbReceptors1);

        // Initialisation of the positions
        buffer = new unsigned int[nbParticle];

        // Creation of the first data model
        ArrayFieldu array1(buffer, nbParticle, 1, false);

        pConstructData container1;
        container1->appendData(std::string("pos"), array1,
                            DECAF_POS, DECAF_PRIVATE,
                            DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        component1->process(container1, decaf::DECAF_REDIST_SOURCE);

    }

    // receiving at the first destination
    if (isBetween(rank, startReceptors1, nbReceptors1))
    {
        fprintf(stderr, "Receiving at Receptor1...\n");
        pConstructData result;
        component1->process(result, decaf::DECAF_REDIST_DEST);
        component1->flush();    // We still need to flush if not doing a get/put

        fprintf(stderr, "===========================\n"
                "Final Merged map has %u items (Expect %llu)\n"
                "Final Merged map has %zu fields (Expect 1)\n",
                result->getNbItems(),
                (nbParticle * nbSource) / nbReceptors1,
                result->getMap()->size());
        result->printKeys();
        //printMap(*result);
        fprintf(stderr, "===========================\n"
                "Simple test between %d producers and %d consumers completed\n",
                nbSource, nbReceptors1);
    }

    if (isBetween(rank, startSource, nbSource))
    {
        component1->flush();
    }

    if (component1) delete component1;
    if (buffer) delete [] buffer;

    fprintf(stderr, "-------------------------------------\n"
            "Test with Redistribution component with overlapping completed\n"
            "-------------------------------------\n");
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

    unsigned long long items;
    items = strtoull(argv[1], NULL, 10);

    fprintf(stderr, "Number of items loaded: %llu\n", items);

    runTestParallel2RedistOverlap(0, 5, 5, 2, items);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
