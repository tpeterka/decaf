//---------------------------------------------------------------------------
//
// Example of redistribution using the Round Redistribution component
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <bredala/transport/mpi/redist_proc_mpi.h>

#include "tools.hpp"

using namespace decaf;
using namespace std;


void runTestParallelRedistOverlap(int startSource, int nbSource, int startReceptors, int nbReceptors)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < max(startSource + nbSource, startReceptors + nbReceptors))
    {
        std::cout<<"ERROR : not enough rank for "<<nbSource<<" sources and "<<nbReceptors<<" receivers"<<std::endl;
        return;
    }

    if(!isBetween(rank, startSource, nbSource) && !isBetween(rank, startReceptors, nbReceptors))
        return;

    std::cout<<"Construction of the redistribution component."<<std::endl;
    std::cout<<"Rank : "<<rank<<std::endl;
    std::cout<<"Configuration : "<<startSource<<", "<<nbSource<<", "<<startReceptors<<", "<<nbReceptors<<std::endl;
    RedistProcMPI *component = new RedistProcMPI(startSource, nbSource,
                                                     startReceptors, nbReceptors,
                                                     MPI_COMM_WORLD, DECAF_REDIST_P2P, DECAF_REDIST_MERGE_ONCE);

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(isBetween(rank, startSource, nbSource)){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        std::vector<int> index;
        index.push_back(rank);


        printArray(index);

        VectorFieldi array(index, 1);
        pConstructData container;



        container->appendData(std::string("index"), array,
                             DECAF_POS, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component->process(container, decaf::DECAF_REDIST_SOURCE);

    }
    if(isBetween(rank, startReceptors, nbReceptors))
    {
        pConstructData result;
        component->process(result, decaf::DECAF_REDIST_DEST);

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        VectorFieldi index = result->getFieldData<VectorFieldi>("index");
        printArray(index.getVector());
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors<<" consummer completed"<<std::endl;
    }

    component->flush();

    delete component;

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping completed"<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;
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

    //runTestParallelRedistOverlap(0, 4, 4, 4);
    //runTestParallelRedistOverlap(0, 4, 4, 3);
    //runTestParallelRedistOverlap(0, 4, 2, 2);
    //runTestParallelRedistOverlap(0, 4, 2, 3);
    //runTestParallelRedistOverlap(0, 4, 0, 4);
    runTestParallelRedistOverlap(0, 4, 4, 2);
    runTestParallelRedistOverlap(0, 4, 0, 4);
    runTestParallelRedistOverlap(0, 4, 3, 4);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
