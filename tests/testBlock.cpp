//---------------------------------------------------------------------------
//
// Example of Redistribution with the ZCurve component
// .ply files are produced for each rank of the consumers
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/blockfield.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/boost_macros.h>

#include <bredala/transport/mpi/redist_block_mpi.h>
#include "tools.hpp"



using namespace decaf;
using namespace std;


void runTestParallelRedistOverlap(int startSource, int nbSource,
                                   int startReceptors, int nbReceptors,
                                   string basename)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < max(startSource + nbSource, startReceptors + nbReceptors))
    {
        cout<<"ERROR : not enough rank for "<<nbSource<<" sources and "<<nbReceptors<<" receivers"<<endl;
        return;
    }


    if(!isBetween(rank, startSource, nbSource) && !isBetween(rank, startReceptors, nbReceptors))
        return;

    vector<float> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    RedistBlockMPI *component = new RedistBlockMPI(startSource, nbSource,
                                                   startReceptors, nbReceptors,
                                                   MPI_COMM_WORLD);

    cout<<"-------------------------------------"<<endl;
    cout<<"Test with Redistribution component with overlapping..."<<endl;
    cout<<"-------------------------------------"<<endl;

    unsigned int r,g,b;
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;


    if(isBetween(rank, startSource, nbSource))
    {
        cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<endl;

        int nbParticle = 1000;
        float* pos = new float[nbParticle * 3];



        initPosition(pos, nbParticle, bbox);

        pConstructData container;

        // Could use the Block constructor  BlockField(Block<3>& block, mapConstruct map = mapConstruct())
        // But would cost a copy
        BlockField domainData(true);    // Initialized the memory
        Block<3>* domainBlock = domainData.getBlock();
        vector<unsigned int> extendsBlock = {0,0,0,10,10,10};
        vector<float> bboxBlock = {0.0,0.0,0.0,10.0,10.0,10.0};
        domainBlock->setGridspace(1.0);
        domainBlock->setGlobalExtends(extendsBlock);
        domainBlock->setLocalExtends(extendsBlock);
        domainBlock->setOwnExtends(extendsBlock);
        domainBlock->setGlobalBBox(bboxBlock);
        domainBlock->setLocalBBox(bboxBlock);
        domainBlock->setOwnBBox(bboxBlock);
        domainBlock->ghostSize_ = 1;

        ArrayFieldf array(pos, 3*nbParticle, 3, false);
        SimpleFieldi data(nbParticle);

        container->appendData(string("nbParticules"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container->appendData(string("pos"), array,
                             DECAF_POS, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", domainData,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
        cout<<"End of construction of the data model."<<endl;
        cout<<"Number of particles in the data model : "<<container->getNbItems()<<endl;


        stringstream filename;
        filename<<basename<<rank<<"_before.ply";
        posToFile(array.getArray(), array.getNbItems(), filename.str(),r,g,b);

        component->process(container, decaf::DECAF_REDIST_SOURCE);

        delete [] pos;
    }

    if(isBetween(rank, startReceptors, nbReceptors))
    {
        pConstructData result;
        component->process(result, decaf::DECAF_REDIST_DEST);
        component->flush();

        cout<<"==========================="<<endl;
        cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<endl;
        cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<endl;
        result->printKeys();
        //printMap(*result);
        cout<<"==========================="<<endl;
        cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors<<" consummer completed"<<endl;


        stringstream filename;
        filename<<basename<<rank<<".ply";
        ArrayFieldf pos = result->getFieldData<ArrayFieldf>("pos");
        posToFile(pos.getArray(), pos.getNbItems(), filename.str(),r,g,b);
    }

    component->flush();

    delete component;

    cout<<"-------------------------------------"<<endl;
    cout<<"Test with Redistribution component with overlapping completed"<<endl;
    cout<<"-------------------------------------"<<endl;
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

    runTestParallelRedistOverlap(0, 4, 4, 4, string("NoOverlap4-4_"));
    runTestParallelRedistOverlap(0, 4, 4, 3, string("NoOverlap4-3_"));
    runTestParallelRedistOverlap(0, 4, 2, 4, string("PartialOverlap4-4_"));
    runTestParallelRedistOverlap(0, 4, 2, 3, string("PartialOverlap4-3_"));
    runTestParallelRedistOverlap(0, 4, 0, 4, string("TotalOverlap4-4_"));
    runTestParallelRedistOverlap(2, 4, 0, 2, string("InverseCons4-2_"));

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
