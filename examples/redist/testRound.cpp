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

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>
#include <decaf/data_model/arrayconstructdata.hpp>
#include <decaf/data_model/boost_macros.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <decaf/transport/mpi/redist_round_mpi.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <sstream>
#include <fstream>

using namespace decaf;
using namespace std;

void printArray(const std::vector<int>& array)
{
    std::cout<<"[";
    for(unsigned int i = 0; i < array.size(); i++)
        std::cout<<array[i]<<",";
    std::cout<<"]"<<std::endl;
}

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

    if((rank < startSource || rank >= startSource + nbSource) &&
        (rank < startReceptors || rank >= startReceptors + nbReceptors))
        return;

    std::cout<<"Construction of the redistribution component."<<std::endl;
    std::cout<<"Rank : "<<rank<<std::endl;
    std::cout<<"Configuration : "<<startSource<<", "<<nbSource<<", "<<startReceptors<<", "<<nbReceptors<<std::endl;
    RedistRoundMPI *component = new RedistRoundMPI(startSource, nbSource,
                                                     startReceptors, nbReceptors,
                                                     MPI_COMM_WORLD);

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(rank >= startSource && rank < startSource + nbSource){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        unsigned int nbParticules = 20;
        std::vector<int> index = std::vector<int>(nbParticules);
        for(int i = 0; i< nbParticules; i++)
            index[i] = i;


        std::shared_ptr<VectorConstructData<int> > array = std::make_shared<VectorConstructData<int> >( index, 1 );

        std::shared_ptr<BaseData> container = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object = dynamic_pointer_cast<ConstructData>(container);

        object->appendData(std::string("index"), array,
                             DECAF_ZCURVEKEY, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component->process(container, decaf::DECAF_REDIST_SOURCE);
        component->flush();
    }
    if(rank >= startReceptors && rank < startReceptors + nbReceptors)
    {
        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component->process(result, decaf::DECAF_REDIST_DEST);
        component->flush();

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        std::shared_ptr<BaseConstructData> data = result->getData("index");
        std::shared_ptr<VectorConstructData<int> > index =
                dynamic_pointer_cast<VectorConstructData<int> >(data);
        printArray(index->getVector());
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors<<" consummer completed"<<std::endl;
    }

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

    runTestParallelRedistOverlap(0, 4, 4, 4);
    runTestParallelRedistOverlap(0, 4, 4, 3);
    runTestParallelRedistOverlap(0, 4, 2, 2);
    runTestParallelRedistOverlap(0, 4, 2, 3);
    runTestParallelRedistOverlap(0, 4, 0, 4);
    runTestParallelRedistOverlap(2, 4, 0, 2);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
