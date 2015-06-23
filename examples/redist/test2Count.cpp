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

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include "../include/ConstructType.hpp"
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <decaf/transport/mpi/redist_zcurve_mpi.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <sstream>
#include <fstream>



using namespace decaf;

void getHeatMapColor(double value, double *red, double *green, double *blue)
{
  const int NUM_COLORS = 4;
  static double color[NUM_COLORS][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };
    // A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.

  int idx1;        // |-- Our desired color will be between these two indexes in "color".
  int idx2;        // |
  double fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

  if(value <= 0)      {  idx1 = idx2 = 0;            }    // accounts for an input <=0
  else if(value >= 1)  {  idx1 = idx2 = NUM_COLORS-1; }    // accounts for an input >=0
  else
  {
    value = value * (NUM_COLORS-1);        // Will multiply value by 3.
    idx1  = floor(value);                  // Our desired color will be after this index.
    idx2  = idx1+1;                        // ... and before this index (inclusive).
    fractBetween = value - double(idx1);    // Distance between the two indexes (0-1).
  }

  *red   = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
  *green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
  *blue  = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return std::max(lower, std::min(n, upper));
}

void posToFile(const std::vector<double>& pos, const std::string filename, double color)
{
    std::ofstream file;
    std::cout<<"Filename : "<<filename<<std::endl;
    file.open(filename.c_str());
    int nbParticules = pos.size() / 3;

    double r,g,b;
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
    file<<"property double x"<<std::endl;
    file<<"property double y"<<std::endl;
    file<<"property double z"<<std::endl;
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
    std::shared_ptr<ArrayConstructData<double> > array = dynamic_pointer_cast<ArrayConstructData<double> >(map.getData("pos"));
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

void initPosition(std::vector<double>& pos, int nbParticule, const std::vector<double>& bbox)
{
    double dx = bbox[3] - bbox[0];
    double dy = bbox[4] - bbox[1];
    double dz = bbox[5] - bbox[2];

    for(int i = 0;i < nbParticule; i++)
    {
        pos.push_back(bbox[0] + ((double)rand() / (double)RAND_MAX) * dx);
        pos.push_back(bbox[1] + ((double)rand() / (double)RAND_MAX) * dy);
        pos.push_back(bbox[2] + ((double)rand() / (double)RAND_MAX) * dz);
    }
}

void runTestParallel2RedistOverlap(int startSource, int nbSource,
                                   int startReceptors1, int nbReceptors1,
                                   int startReceptors2, int nbReceptors2,
                                   std::string basename)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < max( max(startSource + nbSource, startReceptors1 + nbReceptors1),
                         startReceptors2 + nbReceptors2))
    {
        std::cout<<"ERROR : not enough rank for "<<nbSource<<" sources and "<<max(nbReceptors1,nbReceptors2)<<" receivers"<<std::endl;
        return;
    }

    if(rank >= max( max(startSource + nbSource, startReceptors1 + nbReceptors1),
                    startReceptors2 + nbReceptors2))
        return;

    std::vector<double> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    RedistCountMPI *component1 = NULL;
    RedistCountMPI *component2 = NULL;

    if(rank >= min(startSource,startReceptors1) &&
        rank < max(startSource+nbSource,startReceptors1+nbReceptors1) )
        component1 = new RedistCountMPI(startSource, nbSource,
                                                     startReceptors1, nbReceptors1,
                                                     MPI_COMM_WORLD);
    if(rank >= min(startSource,startReceptors2) &&
        rank < max(startSource+nbSource,startReceptors2+nbReceptors2) )
        component2 = new RedistCountMPI(startSource, nbSource,
                                                     startReceptors2, nbReceptors2,
                                                     MPI_COMM_WORLD);

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(rank >= startSource && rank < startSource + nbSource){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors1<<","<<nbReceptors2<<" consummers"<<std::endl;

        std::vector<double> pos;
        int nbParticule = 4000;

        if(rank == startSource)
            initPosition(pos, nbParticule, bbox);
        else
            nbParticule = 0;


        //Sending to the first
        std::shared_ptr<ArrayConstructData<double> > array1 = std::make_shared<ArrayConstructData<double> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data1  = std::make_shared<SimpleConstructData<int> >( nbParticule );

        std::shared_ptr<BaseData> container1 = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object1 = dynamic_pointer_cast<ConstructData>(container1);
        object1->appendData(std::string("nbParticules"), data1,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        object1->appendData(std::string("pos"), array1,
                             DECAF_ZCURVEKEY, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component1->process(container1, decaf::DECAF_REDIST_SOURCE);
        component1->flush();

        //Sending to the second
        std::shared_ptr<ArrayConstructData<double> > array2 = std::make_shared<ArrayConstructData<double> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data2  = std::make_shared<SimpleConstructData<int> >( nbParticule );

        std::shared_ptr<BaseData> container2 = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object2 = dynamic_pointer_cast<ConstructData>(container2);
        object2->appendData(std::string("nbParticules"), data2,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        object2->appendData(std::string("pos"), array2,
                             DECAF_ZCURVEKEY, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component2->process(container2, decaf::DECAF_REDIST_SOURCE);
        component2->flush();


    }
    if(rank >= startReceptors1 && rank < startReceptors1 + nbReceptors1)
    {
        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component1->process(result, decaf::DECAF_REDIST_DEST);

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        result->printKeys();
        //printMap(*result);
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors1<<" consummer completed"<<std::endl;

    }

    if(rank >= startReceptors2 && rank < startReceptors2 + nbReceptors2)
    {
        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component2->process(result, decaf::DECAF_REDIST_DEST);

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        result->printKeys();
        //printMap(*result);
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors2<<" consummer completed"<<std::endl;

    }

    if(component1) delete component1;
    if(component2) delete component2;

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

    runTestParallel2RedistOverlap(0, 4, 4, 1, 5, 1, std::string("NoOverlap4-4_"));

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
