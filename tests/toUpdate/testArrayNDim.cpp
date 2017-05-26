#include <bredala/data_model/array3dconstructdata.hpp>
#include <bredala/data_model/arrayconstructdata.hpp>
#include <bredala/data_model/constructtype.h>
#include <bredala/data_model/block.hpp>
#include <bredala/transport/mpi/redist_block_mpi.h>

#include <fstream>

using namespace decaf;
using namespace std;

/**********************
 * Tool functions     *
 * ********************/

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


void posToFile(std::vector< std::shared_ptr<BaseData> > data, int totalParticles, const std::string filename)
{
    std::ofstream file;
    std::cout<<"Filename : "<<filename<<std::endl;
    file.open(filename.c_str());

    file<<"ply"<<std::endl;
    file<<"format ascii 1.0"<<std::endl;
    file<<"element vertex "<<totalParticles<<std::endl;
    file<<"property float x"<<std::endl;
    file<<"property float y"<<std::endl;
    file<<"property float z"<<std::endl;
    file<<"property uchar red"<<std::endl;
    file<<"property uchar green"<<std::endl;
    file<<"property uchar blue"<<std::endl;
    file<<"end_header"<<std::endl;

    for(unsigned int i = 0; i < data.size(); i++)
    {
        float color = (float)rand() / (float)RAND_MAX;
        float r,g,b;
        getHeatMapColor(color, &r, &g, &b);

        unsigned int ur,ug,ub;
        //ur = (unsigned int)(r * 255.0);
        //ug = (unsigned int)(g * 255.0);
        //ub = (unsigned int)(b * 255.0);
        //ur = clip<unsigned int>(ur, 0, 255);
        //ug = clip<unsigned int>(ug, 0, 255);
        //ub = clip<unsigned int>(ub, 0, 255);
        ur = rand() % 256;
        ug = rand() % 256;
        ub = rand() % 256;
        std::cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<std::endl;

        std::shared_ptr<ConstructData> container =
                std::dynamic_pointer_cast<ConstructData>(data.at(i));

        std::shared_ptr<ArrayConstructData<float> > posData =
                container->getTypedData<ArrayConstructData<float> >("pos");

        float* pos = posData->getArray();
        int nbParticules = posData->getNbItems();
        for(int j = 0; j < nbParticules; j++)
            file<<pos[3*j]<<" "<<pos[3*j+1]<<" "<<pos[3*j+2]
                <<" "<<ur<<" "<<ug<<" "<<ub<<std::endl;
    }
    file.close();
}

void printPos(std::shared_ptr<BaseData> data)
{
    std::shared_ptr<ConstructData> container =
            std::dynamic_pointer_cast<ConstructData>(data);

    std::shared_ptr<ArrayConstructData<float> > posData =
            container->getTypedData<ArrayConstructData<float> >("pos");

    float* pos = posData->getArray();
    int nbParticules = posData->getNbItems();

    for(int i = 0; i < nbParticules; i++)
                std::cout<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2]<<std::endl;
}

void printArray3d(std::shared_ptr<ConstructData> container)
{
    std::shared_ptr<BaseConstructData> field = container->getData("array3d");
    if(!field)
        std::cout<<"ERROR : unable to find the field array3d."<<std::endl;
    std::shared_ptr<Array3DConstructData<float> > constructField =
            dynamic_pointer_cast<Array3DConstructData<float> >(field);
    boost::multi_array<float,3> *array = constructField->getArray();
    int x = array->shape()[0];
    int y = array->shape()[1];
    int z = array->shape()[2];

    std::cout<<"Array : "<<std::endl;

    for(int i = 0; i < x; i++)
    {
        std::cout<<"[";
        for(int j = 0; j < y; j++)
        {
            for(int k = 0; k < z; k++)
            {
                std::cout<<(*array)[i][j][k]<<",";
            }
        }
        std::cout<<"]"<<std::endl;
    }
}

void printArrayByBlock(std::shared_ptr<ConstructData> container)
{
    std::shared_ptr<Array3DConstructData<float> > arrayData =
            container->getTypedData<Array3DConstructData<float> >("array3d");
    std::shared_ptr<BlockConstructData> blockData =
            container->getTypedData<BlockConstructData>("domain_block");

    if(!arrayData || !blockData)
    {
        std::cerr<<"ERROR : domain_block or array3d is not available to print."<<std::endl;
        return;
    }

    Block<3>& block = blockData->getData();
    boost::multi_array<float,3> *array = arrayData->getArray();

    if(block.hasLocalExtends_)
    {
        std::cout<<"Local array : "<<std::endl;
        int x = block.localExtends_[3];
        int y = block.localExtends_[4];
        int z = block.localExtends_[5];

        std::cout<<"Extends of the localArray : ";
        printExtend(block.localExtends_);
        std::cout<<"Extends of the ownArray : ";
        printExtend(block.ownExtends_);
        std::cout<<"Shape of the localArray : "<<array->shape()[0]<<","<<array->shape()[1]<<","<<array->shape()[2]<<std::endl;

        for(int i = 0; i < x; i++)
        {
            std::cout<<"[";
            for(int j = 0; j < y; j++)
            {
                for(int k = 0; k < z; k++)
                {
                    std::cout<<(*array)[i][j][k]<<",";
                }
            }
            std::cout<<"]"<<std::endl;
        }
    }
    else
        std:cout<<"Local extends not available."<<std::endl;

    if(block.hasOwnExtends_)
    {
        std::cout<<"Own array : "<<std::endl;
        int xstart = block.ownExtends_[0] - block.localExtends_[0];
        int xend = xstart + block.ownExtends_[3];
        int ystart = block.ownExtends_[1] - block.localExtends_[1];
        int yend = ystart + block.ownExtends_[4];
        int zstart = block.ownExtends_[2] - block.localExtends_[2];
        int zend = zstart + block.ownExtends_[5];

        for(int i = xstart; i < xend; i++)
        {
            std::cout<<"[";
            for(int j = ystart; j < yend; j++)
            {
                for(int k = zstart; k < zend; k++)
                {
                    std::cout<<(*array)[i][j][k]<<",";
                }
            }
            std::cout<<"]"<<std::endl;
        }

    }
    else
        std::cout<<"Own extends not available."<<std::endl;
}

void testSimpleArray()
{

    std::cout<<"======================================"<<std::endl;
    std::cout<<"= test serialization/deserialization ="<<std::endl;
    std::cout<<"======================================"<<std::endl;
    int x = 5, y = 5, z = 5;
    boost::multi_array<float,3> array(boost::extents[x][y][z]);
    for(int i = 0; i < x; i++)
    {
        for(int j = 0; j < y; j++)
        {
            for(int k = 0; k < z; k++)
            {
                array[i][j][k] = k + j*y + i * y * z;
            }
        }
    }

    std::shared_ptr<Array3DConstructData<float> > data = std::make_shared<Array3DConstructData<float> >(
                &array);

    boost::multi_array<float,3>* array2 = data->getArray();

    std::cout<<"Array before serialization : "<<std::endl;
    std::cout<<"[";
    for(int i = 0; i < x; i++)
    {
        for(int j = 0; j < y; j++)
        {
            for(int k = 0; k < z; k++)
            {
                std::cout<<(*array2)[i][j][k]<<",";
            }
        }
    }
    std::cout<<"]"<<std::endl;


    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    container->appendData("array", data,
                          DECAF_NOFLAG, DECAF_SHARED,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    if(container->serialize())
    {
        int bufferSize;
        char* buffer = container->getOutSerialBuffer(&bufferSize);


        std::shared_ptr<ConstructData> container2 = std::make_shared<ConstructData>();
        if(container2->merge(buffer, bufferSize))
        {
            std::shared_ptr<BaseConstructData> field = container2->getData("array");
            if(!field)
                std::cout<<"ERROR : unable to find the field array."<<std::endl;
            std::shared_ptr<Array3DConstructData<float> > constructField =
                    dynamic_pointer_cast<Array3DConstructData<float> >(field);
            boost::multi_array<float,3> *array3 = constructField->getArray();

            std::cout<<"Array after serialization : "<<std::endl;
            std::cout<<"[";
            for(int i = 0; i < x; i++)
            {
                for(int j = 0; j < y; j++)
                {
                    for(int k = 0; k < z; k++)
                    {
                        std::cout<<(*array3)[i][j][k]<<",";
                    }
                }
            }
            std::cout<<"]"<<std::endl;
        }
    }
    else
        std::cout<<"ERROR : failed to serialized the array"<<std::endl;

    std::cout<<"================================================"<<std::endl;
    std::cout<<"= test serialization/deserialization completed ="<<std::endl;
    std::cout<<"================================================"<<std::endl;
}

void testBlockSplitingArray1D()
{

    std::cout<<"============================"<<std::endl;
    std::cout<<"= testBlockSplitingArray1D ="<<std::endl;
    std::cout<<"============================"<<std::endl;
    std::vector<float> globalBBox = {0.0, 0.0, 0.0, 10.0, 10.0, 10.0};
    std::vector<Block<3> > blocks;
    float gridspace = 1.0;

    //First Block, no ghost
    Block<3> block1;
    block1.setGridspace(gridspace);
    block1.setGlobalBBox(globalBBox);
    std::vector<float> localBBox = {0.0, 0.0, 0.0, 5.0, 5.0, 10.0};
    block1.setLocalBBox(localBBox);
    block1.updateExtends();
    blocks.push_back(block1);

    //Second block
    Block<3> block2;
    block2.setGridspace(gridspace);
    block2.setGlobalBBox(globalBBox);
    localBBox = {5.0, 0.0, 0.0, 5.0, 5.0, 10.0};
    block2.setLocalBBox(localBBox);
    block2.updateExtends();
    blocks.push_back(block2);

    //Third block
    Block<3> block3;
    block3.setGridspace(gridspace);
    block3.setGlobalBBox(globalBBox);
    localBBox = {0.0, 5.0, 0.0, 5.0, 5.0, 10.0};
    block3.setLocalBBox(localBBox);
    block3.updateExtends();
    blocks.push_back(block3);

    //Forth block
    Block<3> block4;
    block4.setGridspace(gridspace);
    block4.setGlobalBBox(globalBBox);
    localBBox = {5.0, 5.0, 0.0, 5.0, 5.0, 10.0};
    block4.setLocalBBox(localBBox);
    block4.updateExtends();
    blocks.push_back(block4);

    // Generation of the particles
    int numParticles = 1000;
    float* particles = new float[numParticles * 3];
    for(int i = 0; i < numParticles; i++)
    {
        particles[3*i] = globalBBox[0] + ((float)rand() / (float)RAND_MAX) * globalBBox[3];
        particles[3*i+1] = globalBBox[1] + ((float)rand() / (float)RAND_MAX) * globalBBox[4];
        particles[3*i+2] = globalBBox[2] + ((float)rand() / (float)RAND_MAX) * globalBBox[5];
    }

    //Building the datamodel
    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    std::shared_ptr<ArrayConstructData<float> > pos =
            std::make_shared<ArrayConstructData<float> >(
                particles, numParticles * 3, 3, false, container->getMap());
    container->appendData("pos", pos,
                          DECAF_POS, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);


    std::vector< std::shared_ptr<BaseData> > splitResult = container->split(blocks);

    /*for(unsigned int i = 0; i < splitResult.size(); i++)
    {
        std::cout<<"Block number "<<i<<" : "<<std::endl;
        printPos(splitResult.at(i));
    }*/

    posToFile(splitResult, numParticles, std::string("outSplitBlock.ply"));

    std::cout<<"======================================"<<std::endl;
    std::cout<<"= testBlockSplitingArray1D completed ="<<std::endl;
    std::cout<<"======================================"<<std::endl;
}

void testBlockSplitingArray3D()
{

    std::cout<<"============================"<<std::endl;
    std::cout<<"= testBlockSplitingArray3D ="<<std::endl;
    std::cout<<"============================"<<std::endl;
    std::vector<float> globalBBox = {0.0, 0.0, 0.0, 10.0, 10.0, 1.0};
    std::vector<Block<3> > blocks;
    float gridspace = 1.0;

    //First Block, no ghost
    Block<3> block1;
    block1.setGridspace(gridspace);
    block1.setGlobalBBox(globalBBox);
    std::vector<float> localBBox = {0.0, 0.0, 0.0, 5.0, 5.0, 1.0};
    block1.setLocalBBox(localBBox);
    block1.updateExtends();
    blocks.push_back(block1);

    //Second block
    Block<3> block2;
    block2.setGridspace(gridspace);
    block2.setGlobalBBox(globalBBox);
    localBBox = {5.0, 0.0, 0.0, 5.0, 5.0, 1.0};
    block2.setLocalBBox(localBBox);
    block2.updateExtends();
    blocks.push_back(block2);

    //Third block
    Block<3> block3;
    block3.setGridspace(gridspace);
    block3.setGlobalBBox(globalBBox);
    localBBox = {0.0, 5.0, 0.0, 5.0, 5.0, 1.0};
    block3.setLocalBBox(localBBox);
    block3.updateExtends();
    blocks.push_back(block3);

    //Forth block
    Block<3> block4;
    block4.setGridspace(gridspace);
    block4.setGlobalBBox(globalBBox);
    localBBox = {5.0, 5.0, 0.0, 5.0, 5.0, 1.0};
    block4.setLocalBBox(localBBox);
    block4.updateExtends();
    blocks.push_back(block4);

    // Generation of the 3D array
    unsigned int x = 10, y = 10, z = 1;
    boost::multi_array<float,3> array(boost::extents[x][y][z]);
    for(int i = 0; i < x; i++)
    {
        for(int j = 0; j < y; j++)
        {
            for(int k = 0; k < z; k++)
            {
                array[i][j][k] = i+j;
            }
        }
    }
    Block<3> blockArray;
    blockArray.setGridspace(gridspace);
    blockArray.setGlobalBBox(globalBBox);
    blockArray.setLocalBBox(globalBBox);
    blockArray.updateExtends();

    std::shared_ptr<Array3DConstructData<float> > data = std::make_shared<Array3DConstructData<float> >(
                &array, blockArray, false);


    //Building the datamodel
    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    container->appendData("array3d", data,
                          DECAF_NOFLAG, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

    std::cout<<"Original array : "<<std::endl;
    printArray3d(container);


    std::vector< std::shared_ptr<BaseData> > splitResult = container->split(blocks);

    for(unsigned int i = 0; i < splitResult.size(); i++)
    {
        std::cout<<"Array number "<<i<<" : "<<std::endl;
        std::shared_ptr<ConstructData> subcontainer =
                std::dynamic_pointer_cast<ConstructData>(splitResult.at(i));
        printArray3d(subcontainer);

        //Pushing the global domain info to merge after
        std::shared_ptr<BlockConstructData> blockData =
                std::make_shared<BlockConstructData>(blockArray, subcontainer->getMap());
        subcontainer->appendData("domain_block", blockData,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    }

    std::cout<<"Merging back the blocks..."<<std::endl;
    for(unsigned int i = 1; i < splitResult.size(); i++)
    {
        splitResult.at(0)->merge(splitResult.at(i));
    }

    std::shared_ptr<ConstructData> mergedContainer =
            std::dynamic_pointer_cast<ConstructData>(splitResult.at(0));
    printArray3d(mergedContainer);

    blocks.clear();

    //First Block, no ghost
    block1.setGridspace(gridspace);
    block1.setGlobalBBox(globalBBox);
    localBBox = {0.0, 0.0, 0.0, 6.0, 6.0, 1.0};
    block1.setLocalBBox(localBBox);
    block1.updateExtends();
    blocks.push_back(block1);

    //Second block
    block2.setGridspace(gridspace);
    block2.setGlobalBBox(globalBBox);
    localBBox = {6.0, 0.0, 0.0, 4.0, 6.0, 1.0};
    block2.setLocalBBox(localBBox);
    block2.updateExtends();
    blocks.push_back(block2);

    //Third block
    block3.setGridspace(gridspace);
    block3.setGlobalBBox(globalBBox);
    localBBox = {0.0, 6.0, 0.0, 6.0, 4.0, 1.0};
    block3.setLocalBBox(localBBox);
    block3.updateExtends();
    blocks.push_back(block3);

    //Forth block
    block4.setGridspace(gridspace);
    block4.setGlobalBBox(globalBBox);
    localBBox = {6.0, 6.0, 0.0, 4.0, 4.0, 1.0};
    block4.setLocalBBox(localBBox);
    block4.updateExtends();
    blocks.push_back(block4);

    std::vector< std::shared_ptr<BaseData> > splitResult2 = container->split(blocks);

    for(unsigned int i = 0; i < splitResult2.size(); i++)
    {
        std::cout<<"Array number "<<i<<" : "<<std::endl;
        std::shared_ptr<ConstructData> subcontainer =
                std::dynamic_pointer_cast<ConstructData>(splitResult2.at(i));
        printArray3d(subcontainer);

        //Pushing the global domain info to merge after
        std::shared_ptr<BlockConstructData> blockData =
                std::make_shared<BlockConstructData>(blockArray, subcontainer->getMap());
        subcontainer->appendData("domain_block", blockData,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    }

    std::cout<<"Merging back the blocks..."<<std::endl;
    for(unsigned int i = 1; i < splitResult.size(); i++)
    {
        splitResult2.at(0)->merge(splitResult2.at(i));
    }

    std::shared_ptr<ConstructData> mergedContainer2 =
            std::dynamic_pointer_cast<ConstructData>(splitResult2.at(0));
    printArray3d(mergedContainer2);

    std::cout<<"======================================"<<std::endl;
    std::cout<<"= testBlockSplitingArray3D completed ="<<std::endl;
    std::cout<<"======================================"<<std::endl;

}

void testRedistBlock()
{
    std::cout<<"==================="<<std::endl;
    std::cout<<"= testRedistBlock ="<<std::endl;
    std::cout<<"==================="<<std::endl;

    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::cout<<"Rank : "<<rank<<std::endl;

    RedistBlockMPI* component1;
    RedistBlockMPI* component2;

    if(rank == 0) //Source code
    {
        component1 = new RedistBlockMPI(0, 1, 1, size_world-2, MPI_COMM_WORLD);
        std::cout<<"Source generating the complete array"<<std::endl;

        // Generation of the 3D array
        unsigned int x = 20, y = 20, z = 1;
        boost::multi_array<float,3> array(boost::extents[x][y][z]);
        for(int i = 0; i < x; i++)
        {
            for(int j = 0; j < y; j++)
            {
                for(int k = 0; k < z; k++)
                {
                    array[i][j][k] = i+j;
                }
            }
        }

        std::vector<float> globalBBox = {0.0, 0.0, 0.0, 20.0, 20.0, 1.0};
        float gridspace = 1.0;
        Block<3> blockArray;
        blockArray.setGridspace(gridspace);
        blockArray.setGlobalBBox(globalBBox);
        blockArray.setLocalBBox(globalBBox);
        blockArray.updateExtends();
        blockArray.ghostSize_ = 1;

        std::shared_ptr<Array3DConstructData<float> > data = std::make_shared<Array3DConstructData<float> >(
                    &array, blockArray, false);


        //Building the datamodel
        std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
        container->appendData("array3d", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

        //Pushing the global domain info for the redistribution component
        std::shared_ptr<BlockConstructData> blockData =
                std::make_shared<BlockConstructData>(blockArray, container->getMap());
        container->appendData("domain_block", blockData,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

        std::cout<<"Original array : "<<std::endl;
        printArray3d(container);

        component1->process(container, decaf::DECAF_REDIST_SOURCE);
        component1->flush();    // We still need to flush if not doing a get/put

        std::cout<<"End of the producer."<<std::endl;

    }
    else if(rank < size_world - 1)   // Destination code
    {
        component1 = new RedistBlockMPI(0, 1, 1, size_world-2, MPI_COMM_WORLD);
        component2 = new RedistBlockMPI(1, size_world-2, size_world-1, 1, MPI_COMM_WORLD);
        std::cout<<"Intermediate consummer receiving a part of the block."<<std::endl;

        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component1->process(result, decaf::DECAF_REDIST_DEST);
        component1->flush();    // We still need to flush if not doing a get/put

        printArrayByBlock(result);

        std::cout<<"Intermediate consummer sending its block to the last rank."<<std::endl;
        component2->process(result, decaf::DECAF_REDIST_SOURCE);

        std::cout<<"End of the intermediate consummer. Forwarding to the final process."<<std::endl;
    }
    else
    {
        component2 = new RedistBlockMPI(1, size_world-2, size_world-1, 1, MPI_COMM_WORLD);
        std::cout<<"Final consummer receiving all the partial blocks and merging them."<<std::endl;

        std::shared_ptr<ConstructData> result = std::make_shared<ConstructData>();
        component2->process(result, decaf::DECAF_REDIST_DEST);
        component2->flush();    // We still need to flush if not doing a get/put

        printArrayByBlock(result);

        std::cout<<"End of the final consummer."<<std::endl;
    }


    std::cout<<"============================="<<std::endl;
    std::cout<<"= testRedistBlock completed ="<<std::endl;
    std::cout<<"============================="<<std::endl;
}

int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);
    srand(3543437);
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0)
    {
        testSimpleArray();
        testBlockSplitingArray1D();
        testBlockSplitingArray3D();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(size_world >= 3)
    {
        testRedistBlock();
    }
    MPI_Finalize();

    return 0;
}
