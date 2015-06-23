//---------------------------------------------------------------------------
//
// example of serialization with FFS and MPI (not supported anymore)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include "ffs.h"
#include "fm.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <iostream>

//using namespace decaf;

typedef struct _particules
{
    int nbParticules;
    float *x;
    float *v;
} particules, *particule_ptr;

FMField particule_list[] = {
    {"nbParticules", "integer", sizeof(int), FMOffset(particule_ptr, nbParticules)},
    {"positions", "float[nbParticules]", sizeof(float), FMOffset(particule_ptr, x)},
    {"speed", "float[nbParticules]", sizeof(float), FMOffset(particule_ptr, v)},
    { NULL, NULL, 0, 0}
};

typedef struct _block
{
    int count;
    particules *p;
} block, *block_ptr;

FMField block_list[] = {
    {"count", "integer", sizeof(int), FMOffset(block_ptr, count)},
    {"particules", "particule[count]", sizeof(particules), FMOffset(block_ptr, p)},
    { NULL, NULL, 0, 0},
};

FMStructDescRec block_format_list[]= {
    {"block", block_list, sizeof(block), NULL},
    {"particule", particule_list, sizeof(particules), NULL},
    { NULL, NULL, 0, NULL},
};


void runTestStruct()
{

    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0){
        std::cout<<"-----------------------------------"<<std::endl;
        std::cout<<"Running test with simple structure"<<std::endl;
        std::cout<<"-----------------------------------"<<std::endl;

        particules p[2];

        p[0].nbParticules = 10;
        p[0].v = new float[p[0].nbParticules];
        p[0].x = new float[p[0].nbParticules];
        for(unsigned int i = 0; i < p[0].nbParticules * 3; i++)
        {
            p[0].v[i] = 1.0;
            p[0].x[i] = 3.0;
        }

        p[1].nbParticules = 5;
        p[1].v = new float[p[1].nbParticules];
        p[1].x = new float[p[1].nbParticules];
        for(unsigned int i = 0; i < p[1].nbParticules; i++)
        {
            p[1].v[i] = 1.5;
            p[1].x[i] = 2.0;
        }

        FMContext fmc = create_FMcontext();
        FFSBuffer buf = create_FFSBuffer();
        FMFormat rec_format;
        char* output;
        int output_size;

        rec_format = FMregister_simple_format(fmc, "first_rec", particule_list, sizeof(particules));
        output = FFSencode(buf, rec_format, p, &output_size);

        std::cout<<"Successful encode. Buffer size : "<<output_size<<std::endl;
        std::cout<<"-----------------------------------"<<std::endl;
        std::cout<<"Test with simple structure completed"<<std::endl;
        std::cout<<"-----------------------------------"<<std::endl;
    }
}

void runTestArrayOfStruct()
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < 2)
    {
        std::cout<<"Not enough processes available. (Min 2)"<<std::endl;
        MPI_Finalize();
    }

    if (rank == 0){

        std::cout<<"Running test with array of simple structure"<<std::endl;

        block simpleBlock;
        simpleBlock.count = 2;
        simpleBlock.p = new particules[simpleBlock.count * sizeof(particules)];

        simpleBlock.p[0].nbParticules = 10;
        simpleBlock.p[0].v = new float[simpleBlock.p[0].nbParticules];
        simpleBlock.p[0].x = new float[simpleBlock.p[0].nbParticules];
        for(unsigned int i = 0; i < simpleBlock.p[0].nbParticules; i++)
        {
            simpleBlock.p[0].v[i] = 1.0;
            simpleBlock.p[0].x[i] = 3.0;
        }

        simpleBlock.p[1].nbParticules = 5;
        simpleBlock.p[1].v = new float[simpleBlock.p[1].nbParticules];
        simpleBlock.p[1].x = new float[simpleBlock.p[1].nbParticules];
        for(unsigned int i = 0; i < simpleBlock.p[1].nbParticules; i++)
        {
            simpleBlock.p[1].v[i] = 1.5;
            simpleBlock.p[1].x[i] = 2.0;
        }

        FMContext fmc = create_FMcontext();
        FFSBuffer buf = create_FFSBuffer();
        FMFormat rec_format;
        char* output;
        int output_size;

        rec_format = FMregister_data_format(fmc, block_format_list);
        std::cout<<"Register of the data format completed"<<std::endl;

        //int lengthServerIDFormat, lenghtServerRepFormat;
        //char* serverIDFormat = get_server_ID_FMformat(rec_format, &lengthServerIDFormat);
        //char* serverRepFormat = get_server_rep_FMformat(rec_format, &lenghtServerRepFormat);

        fflush(stdout);
        output = FFSencode(buf, rec_format, &simpleBlock, &output_size);

        std::cout<<"Successful encode. Buffer size : "<<output_size<<std::endl;
        fflush(stdout);


        FFSTypeHandle first_rec_handle;
        FFSContext fmc_r = create_FFSContext();

        block blockReceiv;
        first_rec_handle = FFSset_fixed_target(fmc_r, &block_format_list[0]);

        FFSdecode(fmc_r, output, (char*)(&blockReceiv));

        std::cout<<"Successful decode."<<std::endl;

        std::cout<<"First set of particule------------------"<<std::endl;
        std::cout<<"Number of particule : "<<blockReceiv.p[0].nbParticules<<std::endl;
        std::cout<<"First position : ["<<blockReceiv.p[0].x[0]<<","<<blockReceiv.p[0].x[1]<<","<<blockReceiv.p[0].x[2]<<"]"<<std::endl;
        std::cout<<"First speed : ["<<blockReceiv.p[0].v[0]<<","<<blockReceiv.p[0].v[1]<<","<<blockReceiv.p[0].v[2]<<"]"<<std::endl;
        std::cout<<"Second set of particule------------------"<<std::endl;
        std::cout<<"Number of particule : "<<blockReceiv.p[1].nbParticules<<std::endl;
        std::cout<<"First position : ["<<blockReceiv.p[1].x[0]<<","<<blockReceiv.p[1].x[1]<<","<<blockReceiv.p[1].x[2]<<"]"<<std::endl;
        std::cout<<"First speed : ["<<blockReceiv.p[1].v[0]<<","<<blockReceiv.p[1].v[1]<<","<<blockReceiv.p[1].v[2]<<"]"<<std::endl;

        std::cout<<"Test with array of simple structure completed"<<std::endl;
    }
}

void runMPITestArrayOfStruct()
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < 2)
    {
        std::cout<<"Not enough processes available. (Min 2)"<<std::endl;
        MPI_Finalize();
    }

    if (rank == 0){

        std::cout<<"Running MPI test with array of simple structure"<<std::endl;

        block simpleBlock;
        simpleBlock.count = 2;
        simpleBlock.p = new particules[simpleBlock.count * sizeof(particules)];

        simpleBlock.p[0].nbParticules = 10;
        simpleBlock.p[0].v = new float[simpleBlock.p[0].nbParticules];
        simpleBlock.p[0].x = new float[simpleBlock.p[0].nbParticules];
        for(unsigned int i = 0; i < simpleBlock.p[0].nbParticules; i++)
        {
            simpleBlock.p[0].v[i] = 1.0;
            simpleBlock.p[0].x[i] = 3.0;
        }

        simpleBlock.p[1].nbParticules = 5;
        simpleBlock.p[1].v = new float[simpleBlock.p[1].nbParticules];
        simpleBlock.p[1].x = new float[simpleBlock.p[1].nbParticules];
        for(unsigned int i = 0; i < simpleBlock.p[1].nbParticules; i++)
        {
            simpleBlock.p[1].v[i] = 1.5;
            simpleBlock.p[1].x[i] = 2.0;
        }

        FMContext fmc = create_FMcontext();
        FFSBuffer buf = create_FFSBuffer();
        FMFormat rec_format;
        char* output;
        int output_size;

        rec_format = FMregister_data_format(fmc, block_format_list);
        std::cout<<"Register of the data format completed"<<std::endl;

        //int lengthServerIDFormat, lenghtServerRepFormat;
        //char* serverIDFormat = get_server_ID_FMformat(rec_format, &lengthServerIDFormat);
        //char* serverRepFormat = get_server_rep_FMformat(rec_format, &lenghtServerRepFormat);

        output = FFSencode(buf, rec_format, &simpleBlock, &output_size);

        std::cout<<"Successful encode. Buffer size : "<<output_size<<std::endl;

        MPI_Send(output, output_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

        std::cout<<"End of the emission of the data"<<std::endl;

        fflush(stdout);

    }

    else if(rank == 1)
    {
        FFSTypeHandle first_rec_handle;
        FFSContext fmc_r = create_FFSContext();

        std::cout<<"Reception of the encoded message"<<std::endl;
        MPI_Status status;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int nitems;
        MPI_Get_count(&status, MPI_BYTE, &nitems);
        char* input = new char[nitems];
        MPI_Recv(input, nitems,
               MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);

        std::cout<<"Message received. Decoding..."<<std::endl;
        block blockReceiv;
        first_rec_handle = FFSset_fixed_target(fmc_r, &block_format_list[0]);

        FFSdecode(fmc_r, input, (char*)&blockReceiv);

        std::cout<<"Successful decode."<<std::endl;

        std::cout<<"First set of particule------------------"<<std::endl;
        std::cout<<"Number of particule : "<<blockReceiv.p[0].nbParticules<<std::endl;
        std::cout<<"First position : ["<<blockReceiv.p[0].x[0]<<","<<blockReceiv.p[0].x[1]<<","<<blockReceiv.p[0].x[2]<<"]"<<std::endl;
        std::cout<<"First speed : ["<<blockReceiv.p[0].v[0]<<","<<blockReceiv.p[0].v[1]<<","<<blockReceiv.p[0].v[2]<<"]"<<std::endl;
        if(blockReceiv.count >= 2){
            std::cout<<"Second set of particule------------------"<<std::endl;
            std::cout<<"Number of particule : "<<blockReceiv.p[1].nbParticules<<std::endl;
            std::cout<<"First position : ["<<blockReceiv.p[1].x[0]<<","<<blockReceiv.p[1].x[1]<<","<<blockReceiv.p[1].x[2]<<"]"<<std::endl;
            std::cout<<"First speed : ["<<blockReceiv.p[1].v[0]<<","<<blockReceiv.p[1].v[1]<<","<<blockReceiv.p[1].v[2]<<"]"<<std::endl;
        }
        std::cout<<"Test with array of simple structure completed"<<std::endl;

        fflush(stdout);
    }

}

void run()
{
  MPI_Init(NULL, NULL);

  //runTestStruct();
  runTestArrayOfStruct();
  //runMPITestArrayOfStruct();
  int size_world, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &size_world);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //std::cout<<"Rank "<<rank<<" has reached the barrier"<<std::endl;
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
}

int main(int argc,
         char** argv)
{
  // run decaf
  run();

  return 0;
}

