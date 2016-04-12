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

#include <decaf/data_model/vectorconstructdata.hpp>
#include <decaf/data_model/simpleconstructdata.hpp>
#include <decaf/data_model/constructtype.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <decaf/transport/mpi/redist_count_mpi.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <sstream>
#include <fstream>

using namespace decaf;
using namespace std;

void getHeatMapColor(double value, double *red, double *green, double *blue)
{
    const int NUM_COLORS = 4;
    static double color[NUM_COLORS][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };
    // A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.

    int idx1;        // |-- Our desired color will be between these two indexes in "color".
    int idx2;        // |
    double fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

    if(value <= 0)       {  idx1 = idx2 = 0;            }    // accounts for an input <=0
    else if(value >= 1)  {  idx1 = idx2 = NUM_COLORS-1; }    // accounts for an input >=0
    else
    {
        value = value * (NUM_COLORS-1);        // Will multiply value by 3.
        idx1  = floor(value);                  // Our desired color will be after this index.
        idx2  = idx1+1;                        // ... and before this index (inclusive).
        fractBetween = value - double(idx1);   // Distance between the two indexes (0-1).
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
    int nbParticles = pos.size() / 3;

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

    std::cout<<"Number of particles to save : "<<nbParticles<<std::endl;
    file<<"ply"<<std::endl;
    file<<"format ascii 1.0"<<std::endl;
    file<<"element vertex "<<nbParticles<<std::endl;
    file<<"property double x"<<std::endl;
    file<<"property double y"<<std::endl;
    file<<"property double z"<<std::endl;
    file<<"property uchar red"<<std::endl;
    file<<"property uchar green"<<std::endl;
    file<<"property uchar blue"<<std::endl;
    file<<"end_header"<<std::endl;
    for(int i = 0; i < nbParticles; i++)
        file<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2] <<" "<<ur<<" "<<ug<<" "<<ub<<std::endl;
    file.close();
}

void printMap(ConstructData& map)
{
    std::shared_ptr<VectorConstructData<double> > array =
        dynamic_pointer_cast<VectorConstructData<double> >(map.getData("pos"));
    std::shared_ptr<SimpleConstructData<int> > data =
        dynamic_pointer_cast<SimpleConstructData<int> >(map.getData("nbParticles"));

    std::cout<<"Number of particle : "<<data->getData()<<std::endl;
    std::cout<<"Positions : "<<std::endl;
    for(unsigned int i = 0; i < array->getVector().size() / 3; i++)
    {
        std::cout<<"["<<array->getVector().at(3*i)
                 <<","<<array->getVector().at(3*i+1)
                 <<","<<array->getVector().at(3*i+2)<<"]"<<std::endl;
    }
}

void initPosition(std::vector<double>& pos, int nbParticle, const std::vector<double>& bbox)
{
    double dx = bbox[3] - bbox[0];
    double dy = bbox[4] - bbox[1];
    double dz = bbox[5] - bbox[2];

    for(int i = 0;i < nbParticle; i++)
    {
        pos.push_back(bbox[0] + ((double)rand() / (double)RAND_MAX) * dx);
        pos.push_back(bbox[1] + ((double)rand() / (double)RAND_MAX) * dy);
        pos.push_back(bbox[2] + ((double)rand() / (double)RAND_MAX) * dz);
    }
}

void runTestParallel2RedistOverlap(int startSource, int nbSource,
                                   int startReceptors1, int nbReceptors1,
                                   int startReceptors2, int nbReceptors2)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size_world < max( max(startSource + nbSource, startReceptors1 + nbReceptors1),
                          startReceptors2 + nbReceptors2))
    {
        std::cout<<"ERROR : not enough rank for "<<nbSource<<" sources and "<<
            max(nbReceptors1,nbReceptors2)<<" receivers"<<std::endl;
        return;
    }

    if (rank >= max( max(startSource + nbSource, startReceptors1 + nbReceptors1),
                     startReceptors2 + nbReceptors2))
        return;

    std::vector<double> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    RedistCountMPI *component1 = NULL;
    RedistCountMPI *component2 = NULL;

    if ((rank >= startSource     && rank < startSource     + nbSource) ||
        (rank >= startReceptors1 && rank < startReceptors1 + nbReceptors1))
        component1 = new RedistCountMPI(startSource,
                                        nbSource,
                                        startReceptors1,
                                        nbReceptors1,
                                        MPI_COMM_WORLD);
    if ((rank >= startSource     && rank < startSource     + nbSource) ||
        (rank >= startReceptors2 && rank < startReceptors2 + nbReceptors2))
        component2 = new RedistCountMPI(startSource,
                                        nbSource,
                                        startReceptors2,
                                        nbReceptors2,
                                        MPI_COMM_WORLD);

    fprintf(stderr, "-------------------------------------\n"
            "Test with Redistribution component with overlapping...\n"
            "-------------------------------------\n");

    if (rank >= startSource && rank < startSource + nbSource)
    {
        fprintf(stderr, "Running Redistribution test between %d producers and %d, %d consumers\n",
                nbSource, nbReceptors1, nbReceptors2);

        std::vector<double> pos;
        int nbParticle = 4000;

        initPosition(pos, nbParticle, bbox);

        // sending to the first destination
        std::shared_ptr<VectorConstructData<double> > array1 =
            std::make_shared<VectorConstructData<double> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data1  =
            std::make_shared<SimpleConstructData<int> >( nbParticle );

        std::shared_ptr<BaseData> container1 = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object1 = dynamic_pointer_cast<ConstructData>(container1);
        object1->appendData(std::string("nbParticles"), data1,
                            DECAF_NOFLAG, DECAF_SHARED,
                            DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        object1->appendData(std::string("pos"), array1,
                            DECAF_ZCURVEKEY, DECAF_PRIVATE,
                            DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component1->process(container1, decaf::DECAF_REDIST_SOURCE);
        component1->flush();    // We still need to flush if not doing a get/put

        // sending to the second destination
        std::shared_ptr<VectorConstructData<double> > array2 =
            std::make_shared<VectorConstructData<double> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data2  =
            std::make_shared<SimpleConstructData<int> >( nbParticle );

        std::shared_ptr<BaseData> container2 = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object2 = dynamic_pointer_cast<ConstructData>(container2);
        object2->appendData(std::string("nbParticles"), data2,
                            DECAF_NOFLAG, DECAF_SHARED,
                            DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        object2->appendData(std::string("pos"), array2,
                            DECAF_ZCURVEKEY, DECAF_PRIVATE,
                            DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component2->process(container2, decaf::DECAF_REDIST_SOURCE);
        component2->flush();    // We still need to flush if not doing a get/put
    }

    // receiving at the first destination
    if (rank >= startReceptors1 && rank < startReceptors1 + nbReceptors1)
    {
        fprintf(stderr, "Receiving at Receptor1...\n");
        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component1->process(result, decaf::DECAF_REDIST_DEST);
        component1->flush();    // We still need to flush if not doing a get/put

        fprintf(stderr, "===========================\n"
                "Final Merged map has %d items\n"
                "Final Merged map has %d fields\n",
                result->getNbItems(), result->getMap()->size());
        result->printKeys();
        //printMap(*result);
        fprintf(stderr, "===========================\n"
                "Simple test between %d producers and %d consumers completed\n",
                nbSource, nbReceptors1);
    }

    // receiving at the second destination
    if (rank >= startReceptors2 && rank < startReceptors2 + nbReceptors2)
    {
        fprintf(stderr, "Receiving at Receptor2...\n");
        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component2->process(result, decaf::DECAF_REDIST_DEST);
        component2->flush();    // We still need to flush if not doing a get/put

        fprintf(stderr, "===========================\n"
                "Final Merged map has %d items\n"
                "Final Merged map has %d fields\n",
                result->getNbItems(), result->getMap()->size());
        result->printKeys();
        //printMap(*result);
        fprintf(stderr, "===========================\n"
                "Simple test between %d producers and %d consumers completed\n",
                nbSource, nbReceptors2);
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

    runTestParallel2RedistOverlap(0, 4, 4, 1, 5, 1);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
