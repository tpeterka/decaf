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
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/boost_macros.h>

#include <bredala/transport/mpi/redist_count_mpi.h>
#include "tools.hpp"


using namespace decaf;
using namespace std;

void runTestParallel2RedistOverlap(int startSource, int nbSource,
                                   int startReceptors1, int nbReceptors1,
                                   int startReceptors2, int nbReceptors2)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!isBetween(rank, startSource, nbSource)
            && !isBetween(rank, startReceptors1, nbReceptors1)
            && !isBetween(rank, startReceptors2, nbReceptors2))
        return;

    std::vector<float> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    RedistCountMPI *component1 = NULL;
    RedistCountMPI *component2 = NULL;
    std::cout<<"Current rank : "<<rank<<std::endl;

    if (isBetween(rank, startSource, nbSource)
            || isBetween(rank, startReceptors1, nbReceptors1))
    {
        std::cout <<"consutrcution for the first component ."<<std::endl;
        component1 = new RedistCountMPI(startSource,
                                        nbSource,
                                        startReceptors1,
                                        nbReceptors1,
                                        MPI_COMM_WORLD,
                                        DECAF_REDIST_P2P);
    }
    if (isBetween(rank, startSource, nbSource)
            || isBetween(rank, startReceptors2, nbReceptors2))
    {
        std::cout<<"Construction of the second component."<<std::endl;
        component2 = new RedistCountMPI(startSource,
                                        nbSource,
                                        startReceptors2,
                                        nbReceptors2,
                                        MPI_COMM_WORLD,
                                        DECAF_REDIST_P2P);
    }

    fprintf(stderr, "-------------------------------------\n"
            "Test with Redistribution component with overlapping...\n"
            "-------------------------------------\n");

    int nbParticle = 40000;

    if (isBetween(rank, startSource, nbSource))
    {
        fprintf(stderr, "Running Redistribution test between %d producers and %d, %d consumers\n",
                nbSource, nbReceptors1, nbReceptors2);

        // Initialisation of the positions
        std::vector<float> pos;
        initPosition(pos, nbParticle, bbox);

        // Creation of the first data model
        VectorFieldf array1(pos, 3);
        SimpleFieldi data1(nbParticle);
        pConstructData container1;
        container1->appendData(std::string("nbParticles"), data1,
                            DECAF_NOFLAG, DECAF_SHARED,
                            DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container1->appendData(std::string("pos"), array1,
                            DECAF_POS, DECAF_PRIVATE,
                            DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        component1->process(container1, decaf::DECAF_REDIST_SOURCE);

        // Creation of the second data model
        VectorFieldf array2(pos, 3);
        SimpleFieldi data2(nbParticle);
        pConstructData container2;
        container2->appendData(std::string("nbParticles"), data2,
                            DECAF_NOFLAG, DECAF_SHARED,
                            DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container2->appendData(std::string("pos"), array2,
                            DECAF_POS, DECAF_PRIVATE,
                            DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        // Redistributing the data
        component2->process(container2, decaf::DECAF_REDIST_SOURCE);

    }

    // receiving at the first destination
    if (isBetween(rank, startReceptors1, nbReceptors1))
    {
        fprintf(stderr, "Receiving at Receptor1...\n");
        pConstructData result;
        component1->process(result, decaf::DECAF_REDIST_DEST);
        component1->flush();    // We still need to flush if not doing a get/put

        fprintf(stderr, "===========================\n"
                "Final Merged map has %u items (Expect %u)\n"
                "Final Merged map has %zu fields (Expect 2)\n",
                result->getNbItems(),
                (nbParticle * nbSource) / nbReceptors1,
                result->getMap()->size());
        result->printKeys();
        //printMap(*result);
        fprintf(stderr, "===========================\n"
                "Simple test between %d producers and %d consumers completed\n",
                nbSource, nbReceptors1);
    }

    // receiving at the second destination
    if (isBetween(rank, startReceptors2, nbReceptors2))
    {
        fprintf(stderr, "Receiving at Receptor2...\n");
        pConstructData result;
        component2->process(result, decaf::DECAF_REDIST_DEST);
        component2->flush();    // We still need to flush if not doing a get/put

        fprintf(stderr, "===========================\n"
                "Final Merged map has %u items (Expect %u)\n"
                "Final Merged map has %zu fields (Expect 2)\n",
                result->getNbItems(),
                (nbParticle * nbSource) / nbReceptors2,
                result->getMap()->size());
        result->printKeys();
        //printMap(*result);
        fprintf(stderr, "===========================\n"
                "Simple test between %d producers and %d consumers completed\n",
                nbSource, nbReceptors2);
    }
    if (isBetween(rank, startSource, nbSource))
    {
        component1->flush();
        component2->flush();
    }

    if (component1) delete component1;
    if (component2) delete component2;

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

    runTestParallel2RedistOverlap(2, 4, 0, 2, 5, 1);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
