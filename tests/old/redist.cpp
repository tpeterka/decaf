//---------------------------------------------------------------------------
//
// example of redistribution with FFS (not supported anymore)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <bredala/transport/mpi/redist_count_mpi.h>
#include <bredala/transport/mpi/types.h>
#include "ffs.h"
#include "fm.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>

using namespace decaf;

typedef struct particules
{
    int nbParticules;
    int sizeArray;
    float *x;
    float *v;

    particules(int nb = 0) //To test if we can switch array for vector with FFS
    {
        nbParticules = nb;
        sizeArray = nb * 3;
        if(nbParticules > 0){
            x = (float*)malloc(sizeArray * sizeof(float));
            v = (float*)malloc(sizeArray * sizeof(float));
        }
    }

    particules(int nb, float* x_, float* v_)
    {
        nbParticules = nb;
        sizeArray = nb * 3;
        x = x_;
        v = v_;
    }

    ~particules()
    {
        free(x);
        free(v);
    }

} *particule_ptr;

FMField particule_list[] = {
    {"nbParticules", "integer", sizeof(int), FMOffset(particule_ptr, nbParticules)},
    {"sizeArray", "integer", sizeof(int), FMOffset(particule_ptr, sizeArray)},
    {"positions", "float[sizeArray]", sizeof(float), FMOffset(particule_ptr, x)},
    {"speed", "float[sizeArray]", sizeof(float), FMOffset(particule_ptr, v)},
    { NULL, NULL, 0, 0},
};

FMStructDescRec particule_format_list[]= {
    {"particule", particule_list, sizeof(particules), NULL},
    { NULL, NULL, 0, NULL},
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

class ParticuleType : public BaseData
{
public:


    ParticuleType(std::shared_ptr<void> data = std::shared_ptr<void>()) : buffer_(NULL),                      // Buffer use for the serialization
        size_buffer_(0), BaseData(data), p(NULL)
    {
        if(data)
        {
            p = (particules*)(data.get());
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
            //nbElements_ = 1;
        }
    }

    virtual ~ParticuleType(){}

    virtual void purgeData()
    {
        nbItems_ = 0;
        splitable_ = false;
        data_ = std::shared_ptr<void>();
    }

    // Return true is some field in the data can be used to
    // compute a Z curve (should be a float* field)
    virtual bool hasZCurveKey()
    {
        if(p == NULL) return false;
        return true;
    }

    // Extract the field containing the positions to compute
    // the ZCurve.
    virtual const float* getZCurveKey(int *nbItems)
    {
        if(p == NULL) return NULL;

        *nbItems = nbItems_;
        return p->x;
    }

    virtual bool hasZCurveIndex(){ return false; }

    virtual bool isSystem(){ return false; }

    virtual const unsigned int* getZCurveIndex(int *nbItems)
    {
        *nbItems = nbItems_;
        return NULL;
    }

    // Return true if the data contains more than 1 item
    virtual bool isSplitable()
    {
        //We need at least 3 elements in the arrays for one particule
        if(p == NULL || p->nbParticules < 1) return false;
        return true;
    }

    virtual std::vector<std::shared_ptr<BaseData> > split(
            const std::vector<int>& range)
    {
        std::vector<std::shared_ptr<BaseData> > result;

        if(p == NULL) return result;

        //Sanity check
        unsigned int sum = 0;
        for(unsigned int i = 0; i < range.size(); i++)
            sum += range.at(i);
        if(sum != nbItems_)
        {
            std::cerr<<"ERROR in class ParticuleType, the sum of ranges "
            <<sum<<" does not match the number of items ("<<nbItems_<<" available."<<std::endl;
            return result;
        }

        int currentLocalItem = 0;

        for(unsigned int i = 0; i < range.size(); i++)
        {
            //Generate the structure and allocate the arrays
            std::shared_ptr<particules> newParticules = std::make_shared<particules>(range.at(i));

            //Filling the structure
            //WARNING : DANGEROUS CODE
            memcpy(newParticules->x,
                   p->x + (currentLocalItem * 3),
                   range.at(i) * 3 * sizeof(float));

            memcpy(newParticules->v,
                   p->v + (currentLocalItem * 3),
                   range.at(i)  * 3 * sizeof(float));

            currentLocalItem += range.at(i);

            std::shared_ptr<BaseData> particuleOnject = make_shared<ParticuleType>(newParticules);

            result.push_back(particuleOnject);
        }

        return result;
    }

    // Split the data in range.size() groups. The group i contains the items
    // with the indexes from range[i]
    // WARNING : All the indexes of the data should be included
    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<std::vector<int> >& range)
    {
        std::vector< std::shared_ptr<BaseData> > result;

        return result;
    }

    virtual bool merge(shared_ptr<BaseData> other)
    {
        return true;
    }

    // Insert the data from other in its serialize form
    virtual bool merge(char* buffer, int size)
    {
        std::cout<<"Merging the data..."<<std::endl;
        FFSTypeHandle first_rec_handle;
        FFSContext fmc_r = create_FFSContext();
        first_rec_handle = FFSset_fixed_target(fmc_r, &particule_format_list[0]);


        //buffer_ = serialdata;
        //size_buffer_ = size;
        std::cout<<"Serial buffer size : "<<size<<std::endl;
        shared_ptr<void> decode_buffer;

        //We don't already have a data. Filling the structure
        if(!data_)
        {
            std::cout<<"No data available in the current object, create one"<<std::endl;
            data_ = shared_ptr<void>(new char[sizeof(particules)],
                    std::default_delete<char[]>());

            decode_buffer = data_;
            std::cout<<"Decoding..."<<std::endl;
            FFSdecode(fmc_r, buffer, (char*)decode_buffer.get());
            std::cout<<"Decoding successful"<<std::endl;

            p = (particules*)(data_.get());

            //nbElements_ = 1;
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
        }
        else //We have to decode elsewhere since the data will be appended
             //to the current data buffer
        {
            std::cout<<"Data available in the current object, create one extra buffer"<<std::endl;
            decode_buffer = shared_ptr<void>(new char[sizeof(particules)],
                                std::default_delete<char[]>());

            std::cout<<"Decoding..."<<std::endl;
            FFSdecode(fmc_r, buffer, (char*)decode_buffer.get());
            std::cout<<"Decoding successful"<<std::endl;

            particules* otherParticule =  (particules*)(decode_buffer.get());

            //Can't realloc the array, we have to deep copy the data

            //Creating the new object
            std::shared_ptr<particules> new_particule = make_shared<particules>(p->nbParticules
                                                                    + otherParticule->nbParticules);
            std::cout<<"The new data has "<<new_particule->nbParticules<<" particules"<<std::endl;
            //Copying the data from the original structure
            memcpy(new_particule->x, p->x, p->sizeArray * sizeof(float));
            memcpy(new_particule->v, p->v, p->sizeArray * sizeof(float));

            //Copying the other structure
            memcpy(new_particule->x + p->nbParticules * 3,
                   otherParticule->x,
                   otherParticule->sizeArray * sizeof(float));
            memcpy(new_particule->v+ p->nbParticules * 3,
                   otherParticule->v,
                   otherParticule->sizeArray * sizeof(float));

            //No need to free the former data, the shared pointer take care of this
            data_ = static_pointer_cast<void>(new_particule);
            p = (particules*)(data_.get());



            //Updating the info of the object
            //nbElements_ = 1;
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
        }
        std::cout<<"Data successfully merged"<<std::endl;
        return true;
    }

    virtual bool merge()
    {
        std::cout<<"Merging the data already in place"<<std::endl;
        FFSTypeHandle first_rec_handle;
        FFSContext fmc_r = create_FFSContext();
        first_rec_handle = FFSset_fixed_target(fmc_r, &particule_format_list[0]);


        std::cout<<"Serial buffer size : "<<size_buffer_<<std::endl;
        shared_ptr<void> decode_buffer;

        //We don't already have a data. Filling the structure
        if(!data_)
        {
            std::cout<<"No data available in the current object, create one"<<std::endl;
            data_ = shared_ptr<void>(new char[sizeof(particules)],
                    std::default_delete<char[]>());

            decode_buffer = data_;
            std::cout<<"Decoding..."<<std::endl;
            FFSdecode(fmc_r, buffer_.get(), (char*)decode_buffer.get());
            std::cout<<"Decoding successful"<<std::endl;

            p = (particules*)(data_.get());

            std::cout<<"Number of particules : "<<p->nbParticules<<std::endl;

            //nbElements_ = 1;
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
        }
        else //We have to decode elsewhere since the data will be appended
             //to the current data buffer
        {
            std::cout<<"Data available in the current object, create one extra buffer"<<std::endl;
            decode_buffer = shared_ptr<void>(new char[sizeof(particules)],
                                std::default_delete<char[]>());

            std::cout<<"Decoding..."<<std::endl;
            FFSdecode(fmc_r, buffer_.get(), (char*)decode_buffer.get());
            std::cout<<"Decoding successful"<<std::endl;

            particules* otherParticule =  (particules*)(decode_buffer.get());

            //Can't realloc the array, we have to deep copy the data

            //Creating the new object
            std::shared_ptr<particules> new_particule = make_shared<particules>(p->nbParticules
                                                                    + otherParticule->nbParticules);
            std::cout<<"The new data has "<<new_particule->nbParticules<<" particules"<<std::endl;
            //Copying the data from the original structure
            memcpy(new_particule->x, p->x, p->sizeArray * sizeof(float));
            memcpy(new_particule->v, p->v, p->sizeArray * sizeof(float));

            //Copying the other structure
            memcpy(new_particule->x + p->nbParticules * 3,
                   otherParticule->x,
                   otherParticule->sizeArray * sizeof(float));
            memcpy(new_particule->v+ p->nbParticules * 3,
                   otherParticule->v,
                   otherParticule->sizeArray * sizeof(float));

            //No need to free the former data, the shared pointer take care of this
            data_ = static_pointer_cast<void>(new_particule);
            p = (particules*)(data_.get());



            //Updating the info of the object
            //nbElements_ = 1;
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
        }
        std::cout<<"Data successfully merged"<<std::endl;
        return true;
    }

    // Update the datamap required to generate the datatype
    virtual bool setData(std::shared_ptr<void> data)
    {
        // We don't have to clean the previous value because it is just
        // a cast to the pointer to the shared memory which is protected
        data_ = data;
        if(data_) //If we have a data
        {
            p = (particules*)(data_.get());
            //nbElements_ = 1;
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
        }
        else
        {
            p = NULL;
            //nbElements_ = 1;
            nbItems_ = 0;
            splitable_ = false;
        }

        return true;
    }

    virtual bool serialize()
    {

        FMContext fmc = create_FMcontext();
        FFSBuffer buf = create_FFSBuffer();
        FMFormat rec_format;

        rec_format = FMregister_data_format(fmc, particule_format_list);

        std::cout<<"Register of the data format completed"<<std::endl;

        fflush(stdout);
        buffer_ = shared_ptr<char>(FFSencode(buf, rec_format, p, &size_buffer_),
                                   std::default_delete<char[]>());
        std::cout<<"Serialization successful (size : "<<size_buffer_<<")"<<std::endl;
        return true;
    }

    virtual bool unserialize()
    {

        return false;
    }

    /*virtual char* getSerialBuffer(int* size){ *size = size_buffer_; return buffer_.get();}
    virtual char* getSerialBuffer(){ return buffer_.get(); }
    virtual int getSerialBufferSize(){ return size_buffer_; }*/

    virtual char* getOutSerialBuffer(int* size)
    {
        *size = size_buffer_; //+1 for the \n caractere
        return buffer_.get(); //Dangerous if the string gets reallocated
    }
    virtual char* getOutSerialBuffer(){ return buffer_.get(); }
    virtual int getOutSerialBufferSize(){ return size_buffer_;}

    virtual char* getInSerialBuffer(int* size)
    {
        *size = size_buffer_; //+1 for the \n caractere
        return buffer_.get(); //Dangerous if the string gets reallocated
    }
    virtual char* getInSerialBuffer(){ return buffer_.get(); }
    virtual int getInSerialBufferSize(){ return size_buffer_; }




    virtual void allocate_serial_buffer(int size)
    {
        buffer_ = std::shared_ptr<char>(new char[size], std::default_delete<char[]>());
        size_buffer_ = size;
    }

    // Merge all the parts we have deserialized so far
    virtual bool mergeStoredData()
    {
        std::cerr<<"ERROR : trying to use mergeStoredData on ParticuleType. Not implemented!"<<std::endl;
        return false;
    }


    // Deserialize the buffer and store the result locally for a later global merge
    virtual void unserializeAndStore(char* buffer, int bufferSize)
    {
        std::cerr<<"ERROR : trying to use unserializeAndStore on ParticuleType. Not implemented!"<<std::endl;
        return ;
    }




protected:

    particules *p;
    std::shared_ptr<char> buffer_;    // Buffer use for the serialization
    int size_buffer_;

};




// user-defined pipeliner code
void pipeliner(Decaf* decaf)
{
}

// user-defined resilience code
void checker(Decaf* decaf)
{
}

// gets command line args
void GetArgs(int argc,
             char **argv,
             DecafSizes& decaf_sizes,
             int& prod_nsteps)
{
  assert(argc >= 9);

  decaf_sizes.prod_size    = atoi(argv[1]);
  decaf_sizes.dflow_size   = atoi(argv[2]);
  decaf_sizes.con_size     = atoi(argv[3]);

  decaf_sizes.prod_start   = atoi(argv[4]);
  decaf_sizes.dflow_start  = atoi(argv[5]);
  decaf_sizes.con_start    = atoi(argv[6]);

  prod_nsteps              = atoi(argv[7]);  // user's, not decaf's variable
  decaf_sizes.con_nsteps   = atoi(argv[8]);
}

void runTestSimple()
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0){
        std::cout<<"Running simple test between one producer and one consummer"<<std::endl;

        std::shared_ptr<particules> p = make_shared<particules>(10);
        for(unsigned int i = 0; i < p->sizeArray; i++)
        {
            p->v[i] = 1.0 * (float)(i+1);
            p->x[i] = 3.0 * (float)(i+1);
        }

        ParticuleType *part = new ParticuleType(static_pointer_cast<void>(p));

        //Serializing the data and send it
        if(part->serialize())
        {
            int buffer_size;
            const char* buffer = part->getOutSerialBuffer(&buffer_size);
            std::cout<<"Size of buffer to send : "<<buffer_size<<std::endl;
            MPI_Send(buffer, buffer_size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        }
        else
        {
            std::cerr<<"ERROR : Enable to serialize the data."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        delete part;

    }
    else if(rank == 1)
    {

        std::cout<<"Reception of the encoded message"<<std::endl;
        MPI_Status status;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int nitems;
        MPI_Get_count(&status, MPI_BYTE, &nitems);

        //NOTE : To modify and use allocate of the data object instead
        std::vector<char> buffer(nitems);
        MPI_Recv(&buffer[0], nitems,
               MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
        std::cout<<"Reception of a message of size "<<nitems<<std::endl;

        // Creating an empty object in which we will uncode the message
        ParticuleType *part = new ParticuleType();
        part->merge(&buffer[0], nitems);

        particules *p = (particules *)(part->getData().get());

        std::cout<<"First set of particule------------------"<<std::endl;
        std::cout<<"Number of particule : "<<p->nbParticules <<std::endl;
        for(unsigned int i = 0; i < p->sizeArray; i+=+3){
            std::cout<<"Position : ["<<p->x[i]<<","<<p->x[i+1]<<","<<p->x[i+2]<<"]"<<std::endl;
            std::cout<<"Speed : ["<<p->v[i]<<","<<p->v[i+1]<<","<<p->v[i+2]<<"]"<<std::endl;
        }
        std::cout<<"Simple test between one producer and one consummer completed"<<std::endl;
       }


}

void runTestSimpleRedist()
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //First 2 ranks are producer, third is consumer
    RedistCountMPI component(0, 2, 2, 1, MPI_COMM_WORLD);

    std::cout<<"Redistribution component initialized."<<std::endl;

    if(rank == 0 or rank == 1){
        std::cout<<"Running simple Redistribution test between two producers and one consummer"<<std::endl;

        std::shared_ptr<particules> p = make_shared<particules>(10);
        for(unsigned int i = 0; i < p->sizeArray; i++)
        {
            p->v[i] = (float)(i+1) + (float)rank * (float)(i+1);
            p->x[i] = (float)(i+1) + (float)rank * (float)(i+1);
        }

        std::shared_ptr<BaseData> data = std::shared_ptr<ParticuleType>(
                    new ParticuleType(static_pointer_cast<void>(p)));

        component.process(data, decaf::DECAF_REDIST_SOURCE);


    }
    else if(rank == 2)
    {
        std::shared_ptr<ParticuleType> result = std::shared_ptr<ParticuleType>(
                    new ParticuleType());
        component.process(result, decaf::DECAF_REDIST_DEST);

        particules *p = (particules *)(result->getData().get());

        std::cout<<"First set of particule------------------"<<std::endl;
        std::cout<<"Number of particule : "<<p->nbParticules<<std::endl;
        for(unsigned int i = 0; i < p->sizeArray; i+=+3){
            std::cout<<"Position : ["<<p->x[i]<<","<<p->x[i+1]<<","<<p->x[i+2]<<"]"<<std::endl;
            std::cout<<"Speed : ["<<p->v[i]<<","<<p->v[i+1]<<","<<p->v[i+2]<<"]"<<std::endl;
        }
        std::cout<<"Simple test between two producers and one consummer completed"<<std::endl;
    }
}

void runTestParallelRedist(int nbSource, int nbReceptors)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < nbSource + nbReceptors)
    {
        std::cout<<"ERROR : not enough rank for "<<nbSource<<" sources and "<<nbReceptors<<" receivers"<<std::endl;
        std::cout<<"World size : "<<size_world<<std::endl;
        return;
    }

    if(rank >= nbSource + nbReceptors)
        return;

    //First 2 ranks are producer, third is consumer
    RedistCountMPI component(0, nbSource, nbSource, nbReceptors, MPI_COMM_WORLD);

    std::cout<<"Redistribution component initialized."<<std::endl;

    if(rank < nbSource){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        std::shared_ptr<particules> p = make_shared<particules>(7);
        for(unsigned int i = 0; i < p->sizeArray; i++)
        {
            p->v[i] = (float)(i+1) + (float)rank * (float)(i+1);
            p->x[i] = (float)(i+1) + (float)rank * (float)(i+1);
        }

        std::shared_ptr<BaseData> data = std::shared_ptr<ParticuleType>(
                    new ParticuleType(static_pointer_cast<void>(p)));

        component.process(data, decaf::DECAF_REDIST_SOURCE);


    }
    else if(rank < nbSource + nbReceptors)
    {
        std::shared_ptr<ParticuleType> result = std::shared_ptr<ParticuleType>(
                    new ParticuleType());
        component.process(result, decaf::DECAF_REDIST_DEST);

        particules *p = (particules *)(result->getData().get());

        std::cout<<"First set of particule------------------"<<std::endl;
        std::cout<<"Number of particule : "<<p->nbParticules<<std::endl;
        for(unsigned int i = 0; i < p->sizeArray; i+=3){
            std::cout<<"Position : ["<<p->x[i]<<","<<p->x[i+1]<<","<<p->x[i+2]<<"]"<<std::endl;
            std::cout<<"Speed : ["<<p->v[i]<<","<<p->v[i+1]<<","<<p->v[i+2]<<"]"<<std::endl;
        }
        std::cout<<"Simple test between two producers and one consummer completed"<<std::endl;
    }
}

void run(DecafSizes& decaf_sizes,
         int prod_nsteps)
{
  MPI_Init(NULL, NULL);

  int size_world, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &size_world);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  runTestParallelRedist(2,1);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
}

int main(int argc,
         char** argv)
{
  // parse command line args
  DecafSizes decaf_sizes;
  int prod_nsteps;
  //GetArgs(argc, argv, decaf_sizes, prod_nsteps);

  // run decaf
  run(decaf_sizes, prod_nsteps);

  return 0;
}
