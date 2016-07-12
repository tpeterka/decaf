//---------------------------------------------------------------------------
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/blockfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <string>

#include "wflow_gromacs.hpp"                         // defines the workflow for this example

#include <decaf/data_model/morton.h>

using namespace decaf;
using namespace std;

void updateGlobalBox(string& profile, BlockField& globalBox, float gridspace)
{
    vector<float> globalPos;
    vector<unsigned int> globalExtends(6);
    Block<3> *block = globalBox.getBlock();

    if(profile.compare(std::string("SimplePeptideWater")) == 0)
    {
        globalPos = {
                        -0.7f, -0.72f, -0.8f,
                        29.0f, 45.0f, 35.17f
                    };
    }
    else if(profile.compare(std::string("bench54k")) == 0)
    {
        globalPos = {
                        0.0f, 0.0f, 0.0f,
                        1283.0f, 1283.0f, 145.0f
                    };
    }
    else
    {
        std::cerr<<"ERROR : unknown profil, can't load a box"<<std::endl;
        exit(1);
    }

    // Extension of the box as these box are the strict box of the first frame
    // so even with some movements the model is still in the box

    float dX = globalPos[3] * 0.2;
    float dY = globalPos[4] * 0.2;
    float dZ = globalPos[5] * 0.2;

    globalPos[0] -= dX;
    globalPos[1] -= dY;
    globalPos[2] -= dZ;
    globalPos[3] += dX;
    globalPos[4] += dY;
    globalPos[5] += dZ;

    globalExtends[0] = 0;
    globalExtends[1] = 0;
    globalExtends[2] = 0;
    globalExtends[3] = (int)(globalPos[3] / gridspace) + 1;
    globalExtends[4] = (int)(globalPos[4] / gridspace) + 1;
    globalExtends[5] = (int)(globalPos[5] / gridspace) + 1;

    block->setGlobalBBox(globalPos);
    block->setGlobalExtends(globalExtends);
    block->setGridspace(gridspace);
    block->setGhostSizes(2);

    globalBox.getBlock()->printBoxes();
    globalBox.getBlock()->printExtends();
}

// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        fprintf(stdout, "dflow gromacs app\n");

        //TODO : find a way to pass the value as a global argument
        BlockField globalBox;
        string model("SimplePeptideWater");
        float gridspace = 1.0;
        updateGlobalBox(model, globalBox, gridspace);

        // We compute the morton codes which will be used
        // for the block redistribution and density grid

        ArrayFieldf posArray = in_data->getFieldData<ArrayFieldf>("pos");
        if(!posArray)
        {
            fprintf(stderr, "ERROR dflow: unable to find the field required \"pos\" in the data model.\n");
            return;
        }


        float* pos = posArray.getArray();
        int nbParticle = posArray->getNbItems();
        fprintf(stdout,"Number of particles in dflow : %i\n", nbParticle);

        for(int i = 0; i < nbParticle; i++)
            pos[i] = pos[i] * 10.0; // Switching from nm to Angstrom

        vector<unsigned int> morton(nbParticle);
        float *box = globalBox.getBlock()->getGlobalBBox();
        unsigned int* cells = globalBox.getBlock()->getGlobalExtends();

        for(int i = 0; i < nbParticle; i++)
        {
            //Using cast from float to unsigned int to keep the lower int
            unsigned int cellX = (unsigned int)((pos[3*i] - box[0]) / gridspace);
            unsigned int cellY = (unsigned int)((pos[3*i+1] - box[1]) / gridspace);
            unsigned int cellZ = (unsigned int)((pos[3*i+2] - box[2]) / gridspace);

            //Clamping the cells to the bbox. Atoms can move away from the box, we count them in the nearest cell (although it's not correct)
            cellX = cellX >= (cells[3])?(cells[3]-1):cellX;
            //cellX = cellX < 0?0:cellX;
            cellY = cellY >= (cells[4])?(cells[4]-1):cellY;
            //cellY = cellY < 0?0:cellY;
            cellZ = cellZ >= (cells[5])?(cells[5]-1):cellZ;
            //cellZ = cellZ < 0?0:cellZ;

            //Computing the corresponding morton code
            morton[i] = Morton_3D_Encode_10bit(cellX,cellY,cellZ);
        }

        ArrayFieldu mortonField = ArrayFieldu(&morton[0], nbParticle, 1, false);

        pConstructData container;
        container->appendData("pos", posArray,
                              DECAF_ZCURVEKEY, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", globalBox,
                              DECAF_NOFLAG, DECAF_SHARED,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
        container->appendData("morton", mortonField,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

         fprintf(stderr, "Sending %i items.\n", container->getNbItems());
         if(container->hasData(string("domain_block")))
             fprintf(stderr, "The block is in the model\n");
         else
             fprintf(stderr, "The block is not in the model\n");

        dataflow->put(container, DECAF_LINK);
    }
} // extern "C"

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                             // workflow
{
    MPI_Init(NULL, NULL);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stdout, "Dflow rank %i\n", rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    //if(argc == 3)
    //{
    //    profile = string(argv[1]);
    //    gridspace = atof(argv[2]);
    //}


    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    // run decaf
    run(workflow);

    return 0;
}
