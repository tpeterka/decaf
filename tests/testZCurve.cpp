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

#include <bredala/data_model/vectorconstructdata.hpp>
#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/boost_macros.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <bredala/transport/mpi/redist_zcurve_mpi.h>

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
  return std::max(lower, std::min(n, upper));
}

void posToFile(const std::vector<float>& pos, const std::string filename, float color)
{
    std::ofstream file;
    std::cout<<"Filename : "<<filename<<std::endl;
    file.open(filename.c_str());
    int nbParticules = pos.size() / 3;

    float r,g,b;
    getHeatMapColor(color, &r, &g, &b);
    std::cout<<"Color : "<<r<<","<<g<<","<<b<<std::endl;

    unsigned int ur,ug,ub;
    ur = (unsigned int)(r * 255.0);
    ug = (unsigned int)(g * 255.0);
    ub = (unsigned int)(b * 255.0);
    ur = clip<unsigned int>(ur, 0, 255);
    ug = clip<unsigned int>(ug, 0, 255);
    ub = clip<unsigned int>(ub, 0, 255);
    std::cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<std::endl;

    std::cout<<"Number of particules to save : "<<nbParticules<<std::endl;
    file<<"ply"<<std::endl;
    file<<"format ascii 1.0"<<std::endl;
    file<<"element vertex "<<nbParticules<<std::endl;
    file<<"property float x"<<std::endl;
    file<<"property float y"<<std::endl;
    file<<"property float z"<<std::endl;
    file<<"property uchar red"<<std::endl;
    file<<"property uchar green"<<std::endl;
    file<<"property uchar blue"<<std::endl;
    file<<"end_header"<<std::endl;
    for(int i = 0; i < nbParticules; i++)
        file<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2]
            <<" "<<ur<<" "<<ug<<" "<<ub<<std::endl;
    file.close();
}


void posToFile(const std::vector<float>& pos, const std::string filename, unsigned int r, unsigned int g, unsigned int b)
{
    std::ofstream file;
    std::cout<<"Filename : "<<filename<<std::endl;
    file.open(filename.c_str());
    int nbParticules = pos.size() / 3;

    unsigned int ur,ug,ub;
    ur = r;
    ug = g;
    ub = b;
    ur = clip<unsigned int>(ur, 0, 255);
    ug = clip<unsigned int>(ug, 0, 255);
    ub = clip<unsigned int>(ub, 0, 255);
    std::cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<std::endl;

    std::cout<<"Number of particules to save : "<<nbParticules<<std::endl;
    file<<"ply"<<std::endl;
    file<<"format ascii 1.0"<<std::endl;
    file<<"element vertex "<<nbParticules<<std::endl;
    file<<"property float x"<<std::endl;
    file<<"property float y"<<std::endl;
    file<<"property float z"<<std::endl;
    file<<"property uchar red"<<std::endl;
    file<<"property uchar green"<<std::endl;
    file<<"property uchar blue"<<std::endl;
    file<<"end_header"<<std::endl;
    for(int i = 0; i < nbParticules; i++)
        file<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2]
            <<" "<<ur<<" "<<ug<<" "<<ub<<std::endl;
    file.close();
}


void printMap(ConstructData& map)
{
    std::shared_ptr<VectorConstructData<float> > array = dynamic_pointer_cast<VectorConstructData<float> >(map.getData("pos"));
    std::shared_ptr<SimpleConstructData<int> > data = dynamic_pointer_cast<SimpleConstructData<int> >(map.getData("nbParticules"));

    std::cout<<"Number of particule : "<<data->getData()<<std::endl;
    std::cout<<"Positions : "<<std::endl;
    for(unsigned int i = 0; i < array->getVector().size() / 3; i++)
    {
        std::cout<<"["<<array->getVector().at(3*i)
                 <<","<<array->getVector().at(3*i+1)
                 <<","<<array->getVector().at(3*i+2)<<"]"<<std::endl;
    }
}

void initPosition(std::vector<float>& pos, int nbParticule, const std::vector<float>& bbox)
{
    float dx = bbox[3] - bbox[0];
    float dy = bbox[4] - bbox[1];
    float dz = bbox[5] - bbox[2];

    for(int i = 0;i < nbParticule; i++)
    {
        pos.push_back(bbox[0] + ((float)rand() / (float)RAND_MAX) * dx);
        pos.push_back(bbox[1] + ((float)rand() / (float)RAND_MAX) * dy);
        pos.push_back(bbox[2] + ((float)rand() / (float)RAND_MAX) * dz);
    }
}

bool isBetween(int rank, int start, int nb)
{
    return rank >= start && rank < start + nb;
}

void runTestParallelRedistOverlap(int startSource, int nbSource,
                                   int startReceptors, int nbReceptors,
                                   std::string basename)
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

    std::vector<float> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    RedistZCurveMPI *component = new RedistZCurveMPI(startSource, nbSource,
                                                     startReceptors, nbReceptors,
                                                     MPI_COMM_WORLD, DECAF_REDIST_COLLECTIVE, DECAF_REDIST_MERGE_STEP, bbox);

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    unsigned int r,g,b;
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;

    if(isBetween(rank, startSource, nbSource)){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        std::vector<float> pos;
        int nbParticule = 1000;


        initPosition(pos, nbParticule, bbox);


        //Sending to the first
        std::shared_ptr<VectorConstructData<float> > array = std::make_shared<VectorConstructData<float> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbParticule );

        pConstructData container = std::make_shared<ConstructData>();

        std::stringstream filename;
        filename<<basename<<rank<<"_before.ply";
        posToFile(array->getVector(), filename.str(),r,g,b);

        container->appendData(std::string("nbParticules"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container->appendData(std::string("pos"), array,
                             DECAF_POS, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component->process(container, decaf::DECAF_REDIST_SOURCE);
    }
    if(isBetween(rank, startReceptors, nbReceptors))
    {
        pConstructData result;
        component->process(result, decaf::DECAF_REDIST_DEST);
        component->flush();

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        result->printKeys();
        //printMap(*result);
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors<<" consummer completed"<<std::endl;


        std::stringstream filename;

        std::shared_ptr<VectorConstructData<float> > pos = result->getTypedData<VectorConstructData<float> >("pos");
        filename<<basename<<rank<<".ply";
        posToFile(pos->getVector(), filename.str(),r,g,b);
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

    runTestParallelRedistOverlap(0, 4, 4, 4, std::string("NoOverlap4-4_"));
    runTestParallelRedistOverlap(0, 4, 4, 3, std::string("NoOverlap4-3_"));
    runTestParallelRedistOverlap(0, 4, 2, 4, std::string("PartialOverlap4-4_"));
    runTestParallelRedistOverlap(0, 4, 2, 3, std::string("PartialOverlap4-3_"));
    runTestParallelRedistOverlap(0, 4, 0, 4, std::string("TotalOverlap4-4_"));
    runTestParallelRedistOverlap(2, 4, 0, 2, std::string("InverseDest4-2_"));


    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
