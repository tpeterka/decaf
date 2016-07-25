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
//#include <decaf/data_model/array3dconstructdata.hpp>
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

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return max(lower, min(n, upper));
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

// consumer
void treatment1(Decaf* decaf)
{
    vector< pConstructData > in_data;
    fprintf(stderr, "Launching treatment\n");
    fflush(stderr);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int iteration = 0;

    while (decaf->get(in_data))
    {
        // get the atom positions
        fprintf(stderr, "Reception of %u messages.\n", in_data.size());

        fprintf(stderr, "Number of particles received : %i\n", in_data[0]->getNbItems());

        //if(iteration == 0 && in_data[0]->getNbItems() > 0)
        //{
        //    fprintf(stderr,"ERROR : not all the treatment got a data. Abording.\n");
        //    MPI_Abort(MPI_COMM_WORLD, 0);
        //}

        if(in_data[0]->getNbItems() > 0)
        {


            // Getting the grid info
            BlockField blockField  = in_data[0]->getFieldData<BlockField>("domain_block");
            Block<3>* block = blockField.getBlock();
            block->printExtends();
            //block->printBoxes();

            ArrayFieldf posField = in_data[0]->getFieldData<ArrayFieldf>("pos");
            float* pos = posField.getArray();
            int nbParticles = posField->getNbItems();

            stringstream filename;
            filename<<"pos_"<<rank<<"_"<<iteration<<"_treat.ply";
            posToFile(pos, nbParticles, filename.str());

            float xmin,ymin,zmin,xmax,ymax,zmax;
            computeBBox(pos, nbParticles,xmin,xmax,ymin,ymax,zmin,zmax);

            MPI_Comm communicator = decaf->con_comm_handle();
            float minl[3];
            float maxl[3];
            float ming[3];
            float maxg[3];

            minl[0] = xmin; minl[1] = ymin; minl[2] = zmin;
            maxl[0] = xmax; maxl[1] = ymax; maxl[2] = zmax;
            MPI_Reduce(minl, ming, 3, MPI_FLOAT, MPI_MIN, 0, communicator);
            MPI_Reduce(maxl, maxg, 3, MPI_FLOAT, MPI_MAX, 0, communicator);

            int localRank;
            MPI_Comm_rank(communicator, &localRank);

            if(localRank == 0)
            {
                std::cout<<"Global box : "<<std::endl;
                std::cout<<"["<<ming[0]<<","<<ming[1]<<","<<ming[2]<<"]["<<maxg[0]<<","<<maxg[1]<<","<<maxg[2]<<"]"<<std::endl;
            }



            // Now each process has a sub domain of the global grid
            //Building the grid
            unsigned int* lExtends = block->getLocalExtends();
            multi_array<float,3>* grid = new multi_array<float,3>(
                        extents[lExtends[3]][lExtends[4]][lExtends[5]]
                    );


            ArrayFieldu mortonField = in_data[0]->getFieldData<ArrayFieldu>("morton");
            unsigned int *morton = mortonField.getArray();
            int nbMorton = mortonField->getNbItems();

            for(int i = 0; i < nbMorton; i++)
            {
                unsigned int x,y,z;
                Morton_3D_Decode_10bit(morton[i], x, y, z);

                // Checking if the particle should be here
                if(!block->isInLocalBlock(x,y,z))
                    fprintf(stderr, "ERROR : particle not belonging to the local block. FIXME\n");

                unsigned int localx, localy, localz;
                localx = x - lExtends[0];
                localy = y - lExtends[1];
                localz = z - lExtends[2];

                (*grid)[localx][localy][localz] += 1.0f; // TODO : get the full formulation
            }
            fprintf(stderr,"Computation of the grid completed\n");

            // Giving the parameter setup to Damaris for the storage layout
            /*if(iteration == 0)
            {
                unsigned int DX,DY,DZ;
                unsigned int* extends = block->getGlobalExtends();
                DX = extends[3]; DY = extends[4]; DZ = extends[5];

                //Pushing the global parameter of the grid
                damaris_parameter_set("DX", &DX, sizeof(unsigned int));
                damaris_parameter_set("DY", &DY, sizeof(unsigned int));
                damaris_parameter_set("DZ", &DZ, sizeof(unsigned int));



                //Creating the scales for the local information
                float* rmesh_x = (float*)malloc(sizeof(float) * (lExtends[3] + 1));
                float* rmesh_y = (float*)malloc(sizeof(float) * (lExtends[4] + 1));
                float* rmesh_z = (float*)malloc(sizeof(float) * (lExtends[5] + 1));

                for(unsigned int i = 0; i <= lExtends[3]; i++)
                    rmesh_x[i] = (float)(lExtends[0]) + (float)(i);
                for(unsigned int i = 0; i <= lExtends[4]; i++)
                    rmesh_y[i] = (float)(lExtends[1]) + (float)(i);
                for(unsigned int i = 0; i <= lExtends[5]; i++)
                    rmesh_z[i] = (float)(lExtends[2]) + (float)(i);

                damaris_write("coordinates/x3d", rmesh_x);
                damaris_write("coordinates/y3d", rmesh_y);
                damaris_write("coordinates/z3d", rmesh_z);

                damaris_end_iteration();

                free(rmesh_x);
                free(rmesh_y);
                free(rmesh_z);

            }*/







            delete grid;
        }


        iteration++;
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "Treatment terminating\n");
    //decaf->terminate();
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
    /*damaris_initialize("decaf_grid.xml",decaf->con_comm_handle());

    int is_client, err;
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
    }*/


    // start the task
    treatment1(decaf);

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
    // define the workflow
    Workflow workflow;
    //make_wflow(workflow);
    Workflow::make_wflow_from_json(workflow, "/examples/gromacs/wflow_gromacs.json");


    // run decaf
    run(workflow);

    return 0;
}
