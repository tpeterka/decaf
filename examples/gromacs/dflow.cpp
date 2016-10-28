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
#include <decaf/data_model/array3dconstructdata.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>

//#include "wflow_gromacs.hpp"                         // defines the workflow for this example
#include <decaf/workflow.hpp>

#include <decaf/data_model/morton.h>

using namespace decaf;
using namespace std;

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return max(lower, min(n, upper));
}

static int iteration = 0;
int filter = 0;

void posToFile(float* pos, int nbParticules, const string filename)
{
    ofstream file;
    //cout<<"Filename : "<<filename<<endl;
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

void updateGlobalBox(string& profile, BlockField& globalBox, float gridspace)
{
    vector<float> globalPos;
    vector<unsigned int> globalExtends(6);
    Block<3> *block = globalBox.getBlock();

    // 02_DA_W.3K
    if(profile.compare(std::string("SimplePeptideWater")) == 0)
    {
        globalPos = {
                        -0.7f, -0.72f, -0.8f,
                        29.0f, 45.0f, 25.4f
                    };
        filter = 191; // We keep only the water
    }
    else if(profile.compare(std::string("bench54k")) == 0)
    {
        globalPos = {
                        0.0f, 0.0f, 0.0f,
                        1283.0f, 1283.0f, 145.0f
                    };
    }
    else if(profile.compare(std::string("fepa")) == 0)
    {
        globalPos = {
                        -10.0f, -10.0f, -10.0f,
                        130.0f, 90.0f, 120.0f
                    };
        filter = 70000; // We keep everything, the Iron complex is
                        // at the end of the frame
    }
    else
    {
        std::cerr<<"ERROR : unknown profil, can't load a box"<<std::endl;
        exit(1);
    }

    // Extension of the box as these box are the strict box of the first frame
    // so even with some movements the model is still in the box

    float dX = ( globalPos[3] ) * 0.2f;
    float dY = ( globalPos[4] ) * 0.2f;
    float dZ = ( globalPos[5] ) * 0.2f;

    globalPos[0] -= dX;
    globalPos[1] -= dY;
    globalPos[2] -= dZ;
    globalPos[3] += 2*dX;
    globalPos[4] += 2*dY;
    globalPos[5] += 2*dZ;

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

    //globalBox.getBlock()->printBoxes();
    //globalBox.getBlock()->printExtends();

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

void computeMorton(Dataflow* dataflow, pConstructData in_data, string& model, float gridspace)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //TODO : find a way to pass the value as a global argument
    BlockField globalBox;
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

    //compteBBox(pos, nbParticle);

    vector<float> filteredPos;
    vector<unsigned int> filteredIds;

    ArrayFieldu indexes = in_data->getFieldData<ArrayFieldu>("ids");
    if(!indexes)
    {
        fprintf(stderr, "ERROR dflow: unable to find the field required \"ids\" in the data model.\n");
        return;
    }
    unsigned int* index = indexes.getArray();

    int nbFilteredPart = 0;
    for(int i = 0; i < nbParticle; i++)
    {
        if(index[i] < filter)
        {
            filteredPos.push_back(pos[3*i] * 10.0); // Switching from nm to Angstrom
            filteredPos.push_back(pos[3*i+1] * 10.0);
            filteredPos.push_back(pos[3*i+2] * 10.0);
            filteredIds.push_back(index[i]);
            nbFilteredPart++;
        }
    }

    ArrayFieldf filterPosField = ArrayFieldf(&filteredPos[0], 3 * nbFilteredPart, 3, false);
    ArrayFieldu filterIdsField = ArrayFieldu(&filteredIds[0], nbFilteredPart, 1, false);

    vector<unsigned int> morton(nbFilteredPart);
    float *box = globalBox.getBlock()->getGlobalBBox();
    unsigned int* cells = globalBox.getBlock()->getGlobalExtends();
    unsigned int offset = 0;

    for(int i = 0; i < nbFilteredPart; i++)
    {
        //Using cast from float to unsigned int to keep the lower int
        unsigned int cellX = (unsigned int)((filteredPos[3*i] - box[0]) / gridspace);
        unsigned int cellY = (unsigned int)((filteredPos[3*i+1] - box[1]) / gridspace);
        unsigned int cellZ = (unsigned int)((filteredPos[3*i+2] - box[2]) / gridspace);

        //Clamping the cells to the bbox. Atoms can move away from the box, we count them in the nearest cell (although it's not correct)
        cellX = cellX >= (cells[3])?(cells[3]-1):cellX;
        //cellX = cellX < 0?0:cellX;
        cellY = cellY >= (cells[4])?(cells[4]-1):cellY;
        //cellY = cellY < 0?0:cellY;
        cellZ = cellZ >= (cells[5])?(cells[5]-1):cellZ;
        //cellZ = cellZ < 0?0:cellZ;

        //Computing the corresponding morton code
        morton[offset] = Morton_3D_Encode_10bit(cellX,cellY,cellZ);
        offset++;
    }

    ArrayFieldu mortonField = ArrayFieldu(&morton[0], nbFilteredPart, 1, false);

    pConstructData container;
    container->appendData("pos", filterPosField,
                          DECAF_POS, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
    container->appendData("domain_block", globalBox,
                          DECAF_NOFLAG, DECAF_SHARED,
                          DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_DEFAULT);
    container->appendData("morton", mortonField,
                          DECAF_MORTON, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
    container->appendData("ids", filterIdsField,
                          DECAF_NOFLAG, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
    dataflow->put(container, DECAF_LINK);

    iteration++;
}

// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow_morton_fepa(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        string model("fepa");
        float gridspace = 2.0;
        computeMorton(dataflow, in_data, model, gridspace);

    }

    void dflow_morton_peptide_water(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        string model("SimplePeptideWater");
        float gridspace = 0.5;
        computeMorton(dataflow, in_data, model, gridspace);

    }

    void dflow_simple(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

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
    fprintf(stdout, "Dflow rank %i\n", rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

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
    //if(argc == 3)
    //{
    //    profile = string(argv[1]);
    //    gridspace = atof(argv[2]);
    //}


    // define the workflow
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "wflow_gromacs.json");

    // run decaf
    run(workflow);

    return 0;
}
