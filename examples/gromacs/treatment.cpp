//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
// this file contains the consumer (2 procs)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/blockfield.hpp>
#include <decaf/data_model/array3dconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <decaf/data_model/boost_macros.h>

#include "decaf/data_model/morton.h"

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <sstream>
#include <fstream>

#include "Damaris.h"

#include <decaf/workflow.hpp>

//#include "wflow_gromacs.hpp"                         // defines the workflow for this example

using namespace decaf;
using namespace std;
using namespace boost;

double* rmesh_x =  NULL;
double* rmesh_y = NULL;
double* rmesh_z = NULL;

int* grid = NULL;

int DX,DY,DZ;

#define GRID(i,j,k) ( grid[i*DY*DZ + j*DZ + k] )


template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return max(lower, min(n, upper));
}

unsigned int lineariseCoord(unsigned int x, unsigned int y, unsigned int z,
                            unsigned int dx, unsigned int dy, unsigned int dz)
{
    //return x * dy * dz + y * dz + z;
    //fprintf(stderr,"access to %u (%u %u %u)\n", x + y*dx + z*dx*dy,x, y, z);
    return x + y*dx + z*dx*dy;
}

std::set<int> filterIds; // Filter selecting the steered group

#define MAX_SIZE_REQUEST 2048
typedef struct
{
    int type;                   //Target absolute in space or relative to a reference point
    float target[3];            //Coordonates of the absolute target
    char targetRequest[2048];   //Request of the
}Target;

std::vector<Target> targets;

std::string model;

void loadTargets()
{
    if(model.compare(std::string("SimplePeptideWater")) == 0)
    {
        Target target;
        target.target[0] = 4.0;
        target.target[1] = 6.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        target.target[0] = 23.0;
        target.target[1] = 6.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        target.target[0] = 23.0;
        target.target[1] = 38.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        target.target[0] = 4.0;
        target.target[1] = 38.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        filterIds = {  109, 110, 111, 112, 113, 114,
                       115, 116, 117, 118, 119, 120
                    }; //HARD CODED for SimpleWater example
    }
    else if(model.compare(std::string("fepa")) == 0)
    {
        Target target;
        target.target[0] = 50.649998;
        target.target[1] = 40.020000;
        target.target[2] = 74.940002;
        targets.push_back(target);

        target.target[0] = 57.994247;
        target.target[1] = 42.744064;
        target.target[2] = 75.205559;
        targets.push_back(target);

        target.target[0] = 58.028599;
        target.target[1] = 39.480324;
        target.target[2] = 62.716755;
        targets.push_back(target);

        target.target[0] = 58.175446;
        target.target[1] = 36.721069;
        target.target[2] = 59.135941;
        targets.push_back(target);

        target.target[0] = 60.568310;
        target.target[1] = 35.987762;
        target.target[2] = 56.373985;
        targets.push_back(target);

        target.target[0] = 57.443069;
        target.target[1] = 41.200779;
        target.target[2] = 52.448627;
        targets.push_back(target);

        target.target[0] = 60.272179;
        target.target[1] = 41.596397;
        target.target[2] = 41.934307;
        targets.push_back(target);

        target.target[0] = 58.013557;
        target.target[1] = 49.347263;
        target.target[2] = 14.191130;
        targets.push_back(target);

        //Ids for ENT and FE residues
        for(int i = 69901; i <= 69952; i++)
        {
            filterIds.insert(i);
        }
        //filterIds.insert(69952);
    }


}

void posToFile(float* pos, int nbParticules, const string filename)
{
    ofstream file;
    cout<<"Filename : "<<filename<<endl;
    file.open(filename.c_str());

    unsigned int r,g,b;
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;

    unsigned int ur,ug,ub;
    ur = r;
    ug = g;
    ub = b;
    ur = clip<unsigned int>(ur, 0, 255);
    ug = clip<unsigned int>(ug, 0, 255);
    ub = clip<unsigned int>(ub, 0, 255);
    //cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<endl;

    //cout<<"Number of particules to save : "<<nbParticules<<endl;
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

void computeBBox(float* pos, int nbParticles,
                 float &xmin, float &xmax,
                 float &ymin, float &ymax,
                 float &zmin, float &zmax)
{
    if(nbParticles > 0)
    {
        xmin = pos[0];
        xmax = pos[0];
        ymin = pos[1];
        ymax = pos[1];
        zmin = pos[2];
        zmax = pos[2];

        for(int i = 1; i < nbParticles; i++)
        {
            if(xmin > pos[3*i])
                xmin = pos[3*i];
            if(ymin > pos[3*i+1])
                ymin = pos[3*i+1];
            if(zmin > pos[3*i+2])
                zmin = pos[3*i+2];
            if(xmax < pos[3*i])
                xmax = pos[3*i];
            if(ymax < pos[3*i+1])
                ymax = pos[3*i+1];
            if(zmax < pos[3*i+2])
                zmax = pos[3*i+2];
        }

        std::cout<<"["<<xmin<<","<<ymin<<","<<zmin<<"]["<<xmax<<","<<ymax<<","<<zmax<<"]"<<std::endl;
    }
}

void compteBBox(float* pos, int nbPos)
{
    if(pos != NULL && nbPos > 0)
    {
        float xmin,ymin,zmin,xmax,ymax,zmax;
        xmin = pos[0];
        ymin = pos[1];
        zmin = pos[2];
        xmax = pos[0];
        ymax = pos[1];
        zmax = pos[2];

        for(int i = 0; i < nbPos; i++)
        {
            if(pos[3*i] < xmin)
                xmin = pos[3*i];
            if(pos[3*i+1] < ymin)
                ymin = pos[3*i+1];
            if(pos[3*i+2] < zmin)
                zmin = pos[3*i+2];
            if(pos[3*i] > xmax)
                xmax = pos[3*i];
            if(pos[3*i+1] > ymax)
                ymax = pos[3*i+1];
            if(pos[3*i+2] > zmax)
                zmax = pos[3*i+2];
        }

        fprintf(stderr, "Local bounding box : [%f %f %f] [%f %f %f]\n", xmin,ymin,zmin,xmax,ymax,zmax);
        fprintf(stderr, "Global bounding box : [%f %f %f] [%f %f %f]\n", -0.7f, -0.72f, -0.8f,
                29.0f, 45.0f, 25.4f);
    }
}

void updateGrid(int* grid, int x, int y, int z, unsigned int DX, unsigned int DY, unsigned int DZ, int increment)
{
    for(int i = -1; i < 2; i++)
    {
        for(int j = -1; j < 2; j++)
        {
            for(int k = -1; k < 2; k++)
            {
                int localX = x+i;
                int localY = y+j;
                int localZ = z+k;
                if(localX >= 0 && localX < DX && localY >= 0 && localY < DY && localZ >= 0 && localZ < DZ)
                    grid[lineariseCoord(localX,localY,localZ,DX,DY,DZ)] += increment;
            }
        }
    }
}


// consumer
void treatment1(Decaf* decaf)
{
    vector< pConstructData > in_data;
    fprintf(stderr, "Launching treatment\n");
    fflush(stderr);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int iteration = 0;

    loadTargets();

    while (decaf->get(in_data))
    {

        //in_data[0]->printKeys();

        // get the atom positions
        if(iteration == 0 && !in_data[0]->hasData("domain_block"))
        {
            fprintf(stderr,"ERROR : No block info in the data model\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        boost::multi_array<float, 3>* boostGrid = NULL;
        if(in_data[0]->hasData("domain_block"))
        {
            // Getting the grid info
            BlockField blockField  = in_data[0]->getFieldData<BlockField>("domain_block");
            Block<3>* block = blockField.getBlock();
            //block->printExtends();
            //block->printBoxes();
            /*if(in_data[0]->getNbItems() > 0)
            {
                ArrayFieldf posField = in_data[0]->getFieldData<ArrayFieldf>("pos");
                float* pos = posField.getArray();
                int nbParticles = posField->getNbItems();

                stringstream filename;
                filename<<"pos_"<<rank<<"_"<<iteration<<"_treat.ply";
                posToFile(pos, nbParticles, filename.str());
            }*/



            // Now each process has a sub domain of the global grid
            // Building the grid
            unsigned int* lExtends = block->getLocalExtends();
            if(!grid)
                grid = new int[lExtends[3]*lExtends[4]*lExtends[5]];
            bzero(grid, lExtends[3]*lExtends[4]*lExtends[5]*sizeof(int));

            boost::multi_array<float, 3>* boostGrid = new boost::multi_array<float, 3>(boost::extents[lExtends[3]][lExtends[4]][lExtends[5]]);

            ArrayFieldu mortonField = in_data[0]->getFieldData<ArrayFieldu>("morton");
            if(mortonField)
            {
                unsigned int *morton = mortonField.getArray();
                int nbMorton = mortonField->getNbItems();
                ArrayFieldf posField = in_data[0]->getFieldData<ArrayFieldf>("pos");
                float* pos = posField.getArray();

                ArrayFieldu idsField = in_data[0]->getFieldData<ArrayFieldu>("ids");
                unsigned int* ids = idsField.getArray();

                for(int i = 0; i < nbMorton; i++)
                {

                    // Building the density grid for the PATH finding
                    // No filtering for this grid as it's not for visualization
                    unsigned int x,y,z;
                    Morton_3D_Decode_10bit(morton[i], x, y, z);

                    int localx, localy, localz;
                    localx = x - lExtends[0];
                    localy = y - lExtends[1];
                    localz = z - lExtends[2];
                    (*boostGrid)[localx][localy][localz] += 1.0f;

                    //Building the density grid for visualization
                    if(filterIds.count(ids[i]) == 0)
                        continue;

                    // Checking if the particle should be here
                    if(!block->isInLocalBlock(x,y,z))
                    {
                        fprintf(stderr, "ERROR : particle not belonging to the local block. FIXME\n");
                        fprintf(stderr, "Particle : %f %f %f\n", pos[3*i], pos[3*i+1], pos[3*i+2]);
                        fprintf(stderr, "Particle : %u %u %u\n", x, y, z);
                        if(block->isInLocalBlock(pos[3*i], pos[3*i+1], pos[3*i+2]))
                            fprintf(stderr, "The particle is in the position block\n");
                        else
                            fprintf(stderr, "The particle is not in the position block\n");

                        float gridspace = blockField.getBlock()->getGridspace();
                        float *box = blockField.getBlock()->getGlobalBBox();
                        //Using cast from float to unsigned int to keep the lower int
                        unsigned int cellX = (unsigned int)((pos[3*i] - box[0]) / gridspace);
                        unsigned int cellY = (unsigned int)((pos[3*i+1] - box[1]) / gridspace);
                        unsigned int cellZ = (unsigned int)((pos[3*i+2] - box[2]) / gridspace);

                        fprintf(stderr," Particle after recomputation : %u %u %u\n", cellX, cellY, cellZ);
                    }

                    updateGrid(grid, localx,localy,localz,lExtends[3],lExtends[4],lExtends[5],1);
                }

                std::shared_ptr<Array3DConstructData<float> > fieldGrid =
                        std::make_shared<Array3DConstructData<float> >(boostGrid, *block, false);

                in_data[0]->appendData("grid", fieldGrid,
                                       DECAF_NOFLAG, DECAF_PRIVATE,
                                       DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

                // Adding the cells of the targets
                for(unsigned int i = 0; i < targets.size(); i++)
                {
                    if(block->isInLocalBlock(targets[i].target[0], targets[i].target[1],targets[i].target[2]))
                    {
                        float gridspace = blockField.getBlock()->getGridspace();
                        float *box = blockField.getBlock()->getGlobalBBox();
                        //Using cast from float to unsigned int to keep the lower int
                        unsigned int cellX = (unsigned int)((targets[i].target[0] - box[0]) / gridspace);
                        unsigned int cellY = (unsigned int)((targets[i].target[1] - box[1]) / gridspace);
                        unsigned int cellZ = (unsigned int)((targets[i].target[2] - box[2]) / gridspace);

                        int localx, localy, localz;
                        localx = cellX - lExtends[0];
                        localy = cellY - lExtends[1];
                        localz = cellZ - lExtends[2];

                        updateGrid(grid, localx,localy,localz,lExtends[3],lExtends[4],lExtends[5], 10);
                    }
                }

            }

            int counter = 0;
            for(unsigned int i = 0; i < lExtends[3]*lExtends[4]*lExtends[5]; i++)
            {
                if(grid[i] > 0)
                    counter++;
            }

            //fprintf(stderr, "Number of active cells : %i\n", counter);
            //fprintf(stderr, "Total number of cells : %u\n", (lExtends[3]*lExtends[4]*lExtends[5]));


            //fprintf(stderr,"Computation of the grid completed\n");

            // Giving the parameter setup to Damaris for the storage layout
            if(iteration == 0)
            {
                unsigned int* extends = block->getLocalExtends();
                DX = extends[3]; DY = extends[4]; DZ = extends[5];

                //Pushing the global parameter of the grid
                damaris_parameter_set("WIDTH", &DX, sizeof(int));
                damaris_parameter_set("HEIGHT", &DY, sizeof(int));
                damaris_parameter_set("DEPTH", &DZ, sizeof(int));



                //Creating the scales for the local information
                rmesh_x = new double[lExtends[3]];
                rmesh_y = new double[lExtends[4]];
                rmesh_z = new double[lExtends[5]];

                for(unsigned int i = 0; i < lExtends[3]; i++)
                    rmesh_x[i] = (double)(lExtends[0]) + (double)(i);
                for(unsigned int i = 0; i < lExtends[4]; i++)
                    rmesh_y[i] = (double)(lExtends[1]) + (double)(i);
                for(unsigned int i = 0; i < lExtends[5]; i++)
                    rmesh_z[i] = (double)(lExtends[2]) + (double)(i);

                damaris_write("coord/x", rmesh_x);
                damaris_write("coord/y", rmesh_y);
                damaris_write("coord/z", rmesh_z);
            }

            // Normal data transmition to Damaris

            damaris_write("space", grid );
            damaris_end_iteration();
        }



        decaf->put(in_data[0]);

        if(boostGrid) delete boostGrid;

        iteration++;
    }

    // Cleaning the grid
    delete [] rmesh_x;
    delete [] rmesh_y;
    delete [] rmesh_z;

    delete [] grid;

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "Treatment terminating\n");
    decaf->terminate();
}

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                             // workflow
{
    MPI_Init(NULL, NULL);

    char processorName[MPI_MAX_PROCESSOR_NAME];
    int size_world, rank, nameLen;

    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processorName,&nameLen);

    srand(time(NULL) + rank * size_world + nameLen);

    fprintf(stderr, "treatment rank %i\n", rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    //Initalizing the Damaris context
    damaris_initialize("decaf_grid.xml",decaf->con_comm_handle());

    int is_client, err;
    err = damaris_start(&is_client);

    if((err == DAMARIS_OK || err == DAMARIS_NO_SERVER) && is_client) {
        MPI_Comm damaris_com;
        damaris_client_comm_get(&damaris_com);

        int decaf_comm_size, damaris_comm_size;
        MPI_Comm_size(damaris_com, &damaris_comm_size);
        MPI_Comm_size(decaf->con_comm_handle(), &decaf_comm_size);
        if(decaf_comm_size != damaris_comm_size)
        {
            fprintf(stderr, "ERROR : Damaris configured to use helper cores.");
            fprintf(stderr, "Set cores and nodes to 0 in decaf_grid.xml\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
    }
    else
    {
        fprintf(stderr, "ERROR during the initialization of Damaris. Abording.\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }


    // start the task
    treatment1(decaf);

    damaris_stop();

    // cleanup
    delete decaf;
    fprintf(stderr,"Decaf deleted. Waiting on finalize\n");
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    fprintf(stderr, "Hello treatment\n");

    if(argc != 2)
    {
        fprintf(stderr, "Usage : treatment profile\n");
        exit(0);
    }

    model = string(argv[1]);

    // define the workflow
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "wflow_gromacs.json");


    // run decaf
    run(workflow);

    return 0;
}
