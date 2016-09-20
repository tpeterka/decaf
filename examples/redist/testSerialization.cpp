#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/vectorfield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <mpi.h>

#define ARRAYSIZE 20

using namespace std;
using namespace decaf;

void printArray(float* array, unsigned int size)
{
    fprintf(stderr,"[ ");
    for(unsigned int i = 0; i < size; i++)
        fprintf(stderr, "%f ", array[i]);
    fprintf(stderr, "]\n");
}

// Simple serialization and deserialization of common data types
void simpleDataModelCreation()
{
    std::vector<float> simplevector(ARRAYSIZE);
    float simplearray[ARRAYSIZE];

    for(unsigned int i = 0; i < ARRAYSIZE; i++)
    {
        simplevector[i] = (float)i;
        simplearray[i] = (float)i;
    }

    SimpleFieldi simplefield(5);
    VectorFieldf vectorfield(simplevector, 1);
    ArrayFieldf arrayfield(simplearray, ARRAYSIZE, 1, false);


    pConstructData container;

    container->appendData(string("simple"), simplefield,
                          DECAF_NOFLAG, DECAF_SHARED,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    container->appendData(string("vector"), vectorfield,
                          DECAF_NOFLAG, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
    container->appendData(string("array"), arrayfield,
                          DECAF_NOFLAG, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

    // Serialization
    container->serialize();

    //Retrieving the serialized buffer
    unsigned int bufferSize = container->getOutSerialBufferSize();
    char* buffer = container->getOutSerialBuffer();

    // Creating a new data model and using the buffer
    pConstructData newcontainer;
    newcontainer->merge(buffer, bufferSize);

    SimpleFieldi othersimplefield = newcontainer->getFieldData<SimpleFieldi>("simple");
    if(!othersimplefield)
    {
        fprintf(stderr, "ERROR : unable to retrieve the field \"simple\"\n");
        exit(1);
    }
    else
        fprintf(stderr, "simple value : %i\n", othersimplefield.getData());

    VectorFieldf othervectorfield = newcontainer->getFieldData<VectorFieldf>("vector");
    if(!othervectorfield)
    {
        fprintf(stderr, "ERROR : unable to retrieve the field \"vector\"\n");
        exit(1);
    }
    else
    {
        vector<float> data = othervectorfield.getVector();
        printArray(&data[0], data.size());
    }

    ArrayFieldf otherarrayfield = newcontainer->getFieldData<ArrayFieldf>("array");
    if(!otherarrayfield)
    {
        fprintf(stderr, "ERROR : unable to retrieve the field \"array\"\n");
        exit(1);
    }
    else
    {
        float* data = otherarrayfield.getArray();
        printArray(&data[0], otherarrayfield.getArraySize());
    }
}

// Serialization and deserialization of custom type
typedef struct
{
    float x;
    float y;
    float z;
    unsigned int id;
} particle;

// Macro for serialization
BOOST_IS_BITWISE_SERIALIZABLE(particle) // Useful if the structure
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<particle>,"ArrayConstructData<particle>")
typedef decaf::ArrayConstructData<particle> ArrayFieldp;


void customDataModel()
{
    particle simplearray[ARRAYSIZE];
    for(unsigned int i = 0; i < ARRAYSIZE; i++)
    {
        simplearray[i].x = (float)rand() / (float)RAND_MAX;
        simplearray[i].y = (float)rand() / (float)RAND_MAX;
        simplearray[i].z = (float)rand() / (float)RAND_MAX;
        simplearray[i].id = i;
    }

    ArrayFieldp arrayfield(simplearray, ARRAYSIZE, 1, false);
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

    simpleDataModelCreation();

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
