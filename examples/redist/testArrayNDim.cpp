#include <decaf/data_model/arrayndimension.hpp>
#include <decaf/transport/mpi/redist_zcurve_mpi.h>
#include <decaf/data_model/constructtype.h>

using namespace decaf;
using namespace std;

void testSimpleArray()
{
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

    std::shared_ptr<ArrayNDimConstructData<float,3> > data = make_shared<ArrayNDimConstructData<float,3> >(
                array);

    boost::multi_array<float,3> array2 = data->getArray();

    std::cout<<"Array2 : "<<std::endl;
    std::cout<<"[";
    for(int i = 0; i < x; i++)
    {
        for(int j = 0; j < y; j++)
        {
            for(int k = 0; k < z; k++)
            {
                std::cout<<array2[i][j][k]<<",";
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
            std::shared_ptr<ArrayNDimConstructData<float,3> > constructField =
                    dynamic_pointer_cast<ArrayNDimConstructData<float,3> >(field);
            boost::multi_array<float,3> array3 = constructField->getArray();

            std::cout<<"Array3 : "<<std::endl;
            std::cout<<"[";
            for(int i = 0; i < x; i++)
            {
                for(int j = 0; j < y; j++)
                {
                    for(int k = 0; k < z; k++)
                    {
                        std::cout<<array3[i][j][k]<<",";
                    }
                }
            }
            std::cout<<"]"<<std::endl;
        }
    }
    else
        std::cout<<"ERROR : failed to serialized the array"<<std::endl;
}

int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);

    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0)
        testSimpleArray();
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
