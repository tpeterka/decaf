#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include "../include/ConstructType.hpp"
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <decaf/transport/mpi/redist_zcurve_mpi.hpp>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <sstream>

using namespace decaf;


void printMorton(int nbSlices)
{
    std::cout<<"Print of the morton codes : "<<std::endl;
    for(unsigned int i = 0; i < nbSlices; i++)
    {
        for(unsigned int j = 0; j < nbSlices; j++)
        {
            for(unsigned int k = 0; k < nbSlices; k++)
            {
                std::cout<<"Morton ["<<i<<","<<j<<","<<k<<"] =>" << Morton_3D_Encode_10bit(i,j,k)<<std::endl;
            }
        }
    }
}



void printMap(ConstructData& map)
{
    std::shared_ptr<ArrayConstructData<float> > array = dynamic_pointer_cast<ArrayConstructData<float> >(map.getData("pos"));
    std::shared_ptr<SimpleConstructData<int> > data = dynamic_pointer_cast<SimpleConstructData<int> >(map.getData("nbParticules"));

    std::cout<<"Number of particule : "<<data->getData()<<std::endl;
    std::cout<<"Positions : "<<std::endl;
    for(unsigned int i = 0; i < array->getArray().size() / 3; i++)
    {
        std::cout<<"["<<array->getArray().at(3*i)
                 <<","<<array->getArray().at(3*i+1)
                 <<","<<array->getArray().at(3*i+2)<<"]"<<std::endl;
    }
}

void initPosition(std::vector<float>& pos, int nbParticule, const std::vector<float>& bbox)
{
    float dx = bbox[3] - bbox[0];
    float dy = bbox[4] - bbox[1];
    float dz = bbox[5] - bbox[2];

    srand(time(NULL));
    for(int i = 0;i < nbParticule; i++)
    {
        pos.push_back(bbox[0] + ((float)rand() / (float)RAND_MAX) * dx);
        pos.push_back(bbox[1] + ((float)rand() / (float)RAND_MAX) * dy);
        pos.push_back(bbox[2] + ((float)rand() / (float)RAND_MAX) * dz);
    }
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

    if(rank >= max(startSource + nbSource, startReceptors + nbReceptors))
        return;

    std::vector<float> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    RedistZCurveMPI *component = new RedistZCurveMPI(startSource, nbSource,
                                                     startReceptors, nbReceptors,
                                                     MPI_COMM_WORLD, bbox);

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(rank >= startSource && rank < startSource + nbSource){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        std::vector<float> pos;
        int nbParticule = 40;


        initPosition(pos, nbParticule, bbox);

        std::shared_ptr<ArrayConstructData<float> > array = std::make_shared<ArrayConstructData<float> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbParticule );


        std::shared_ptr<BaseData> container = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object = dynamic_pointer_cast<ConstructData>(container);
        object->appendData(std::string("nbParticules"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        object->appendData(std::string("pos"), array,
                             DECAF_ZCURVEKEY, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component->process(container, decaf::DECAF_REDIST_SOURCE);
    }
    if(rank >= startReceptors && rank < startReceptors + nbReceptors)
    {
        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component->process(result, decaf::DECAF_REDIST_DEST);

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        result->printKeys();
        printMap(*result);
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

    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //if (rank == 0)
    //    printMorton(4);
    runTestParallelRedistOverlap(0, 4, 4 , 3);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
