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

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <decaf/data_model/simpleconstructdata.hpp>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/arrayconstructdata.hpp>
#include <decaf/data_model/blockconstructdata.hpp>
#include <decaf/data_model/boost_macros.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <decaf/transport/mpi/redist_block_mpi.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <sstream>
#include <fstream>



using namespace decaf;
using namespace std;

void getHeatMapColor(float value, float *red, float *green, float *blue)
{
  const int NUM_COLORS = 4;
  static float color[NUM_COLORS][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };
    // A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.

  int idx1;        // |-- Our desired color will be between these two indexes in "color".
  int idx2;        // |
  float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

  if(value <= 0)      {  idx1 = idx2 = 0;            }    // accounts for an input <=0
  else if(value >= 1)  {  idx1 = idx2 = NUM_COLORS-1; }    // accounts for an input >=0
  else
  {
    value = value * (NUM_COLORS-1);        // Will multiply value by 3.
    idx1  = floor(value);                  // Our desired color will be after this index.
    idx2  = idx1+1;                        // ... and before this index (inclusive).
    fractBetween = value - float(idx1);    // Distance between the two indexes (0-1).
  }

  *red   = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
  *green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
  *blue  = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return max(lower, min(n, upper));
}

void posToFile(float* pos, int nbParticules, const string filename, float color)
{
    ofstream file;
    cout<<"Filename : "<<filename<<endl;
    file.open(filename.c_str());

    float r,g,b;
    getHeatMapColor(color, &r, &g, &b);
    cout<<"Color : "<<r<<","<<g<<","<<b<<endl;

    unsigned int ur,ug,ub;
    ur = (unsigned int)(r * 255.0);
    ug = (unsigned int)(g * 255.0);
    ub = (unsigned int)(b * 255.0);
    ur = clip<unsigned int>(ur, 0, 255);
    ug = clip<unsigned int>(ug, 0, 255);
    ub = clip<unsigned int>(ub, 0, 255);
    cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<endl;

    cout<<"Number of particules to save : "<<nbParticules<<endl;
    file<<"ply"<<endl;
    file<<"format ascii 1.0"<<endl;
    file<<"element vertex "<<nbParticules<<endl;
    file<<"property float x"<<endl;
    file<<"property float y"<<endl;
    file<<"property float z"<<endl;
    file<<"property uchar red"<<endl;
    file<<"property uchar green"<<endl;
    file<<"property uchar blue"<<endl;
    file<<"end_header"<<endl;
    for(int i = 0; i < nbParticules; i++)
        file<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2]
            <<" "<<ur<<" "<<ug<<" "<<ub<<endl;
    file.close();
}


void posToFile(float* pos, int nbParticules, const string filename, unsigned int r, unsigned int g, unsigned int b)
{
    ofstream file;
    cout<<"Filename : "<<filename<<endl;
    file.open(filename.c_str());

    unsigned int ur,ug,ub;
    ur = r;
    ug = g;
    ub = b;
    ur = clip<unsigned int>(ur, 0, 255);
    ug = clip<unsigned int>(ug, 0, 255);
    ub = clip<unsigned int>(ub, 0, 255);
    cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<endl;

    cout<<"Number of particules to save : "<<nbParticules<<endl;
    file<<"ply"<<endl;
    file<<"format ascii 1.0"<<endl;
    file<<"element vertex "<<nbParticules<<endl;
    file<<"property float x"<<endl;
    file<<"property float y"<<endl;
    file<<"property float z"<<endl;
    file<<"property uchar red"<<endl;
    file<<"property uchar green"<<endl;
    file<<"property uchar blue"<<endl;
    file<<"end_header"<<endl;
    for(int i = 0; i < nbParticules; i++)
        file<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2]
            <<" "<<ur<<" "<<ug<<" "<<ub<<endl;
    file.close();
}


void printMap(ConstructData& map)
{
    shared_ptr<VectorConstructData<float> > array = dynamic_pointer_cast<VectorConstructData<float> >(map.getData("pos"));
    shared_ptr<SimpleConstructData<int> > data = dynamic_pointer_cast<SimpleConstructData<int> >(map.getData("nbParticules"));

    cout<<"Number of particule : "<<data->getData()<<endl;
    cout<<"Positions : "<<endl;
    for(unsigned int i = 0; i < array->getVector().size() / 3; i++)
    {
        cout<<"["<<array->getVector().at(3*i)
                 <<","<<array->getVector().at(3*i+1)
                 <<","<<array->getVector().at(3*i+2)<<"]"<<endl;
    }
}

void initPosition(float* pos, int nbParticule, const vector<float>& bbox)
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

bool isBetween(int rank, int start, int nb)
{
    return rank >= start && rank < start + nb;
}

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

        int nbParticule = 1000;
        float* pos = new float[nbParticule * 3];



        initPosition(pos, nbParticule, bbox);

        Block<3> domainBlock;
        vector<unsigned int> extendsBlock = {0,0,0,10,10,10};
        vector<float> bboxBlock = {0.0,0.0,0.0,10.0,10.0,10.0};
        domainBlock.setGridspace(1.0);
        domainBlock.setGlobalExtends(extendsBlock);
        domainBlock.setLocalExtends(extendsBlock);
        domainBlock.setOwnExtends(extendsBlock);
        domainBlock.setGlobalBBox(bboxBlock);
        domainBlock.setLocalBBox(bboxBlock);
        domainBlock.setOwnBBox(bboxBlock);
        domainBlock.ghostSize_ = 1;


        //Sending to the first

        pConstructData container;

        shared_ptr<ArrayConstructData<float> > array =
                make_shared<ArrayConstructData<float> >( pos, 3*nbParticule, 3, false, container->getMap() );
        shared_ptr<SimpleConstructData<int> > data  =
                make_shared<SimpleConstructData<int> >( nbParticule, container->getMap());
        shared_ptr<BlockConstructData> domainData =
                        make_shared<BlockConstructData>(domainBlock, container->getMap());



        stringstream filename;
        filename<<basename<<rank<<"_before.ply";
        posToFile(array->getArray(), array->getNbItems(), filename.str(),r,g,b);

        container->appendData(string("nbParticules"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container->appendData(string("pos"), array,
                             DECAF_POS, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", domainData,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
        cout<<"End of construction of the data model."<<endl;
        cout<<"Number of particles in the data model : "<<container->getNbItems()<<endl;

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
        shared_ptr<BaseConstructData> data = result->getData("pos");
        shared_ptr<ArrayConstructData<float> > pos =
                dynamic_pointer_cast<ArrayConstructData<float> >(data);
        filename<<basename<<rank<<".ply";
        posToFile(pos->getArray(), pos->getNbItems(), filename.str(),r,g,b);
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
