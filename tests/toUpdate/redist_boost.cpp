//---------------------------------------------------------------------------
//
// Example of redistribution with Boost serialization
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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <bredala/data_model/constructtype.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>

using namespace decaf;


class particules
{
public:
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
        if(sizeArray > 0){
            free(x);
            free(v);
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & nbParticules;
        ar & sizeArray;
        ar & boost::serialization::make_array<float>(x,sizeArray);
        ar & boost::serialization::make_array<float>(v,sizeArray);
    }

};

namespace boost { namespace serialization {
template<class Archive>
inline void save_construct_data(
    Archive & ar, const particules * t, const unsigned int file_version
){
    // save data required to construct instance
    ar << t->nbParticules;
}

template<class Archive>
inline void load_construct_data(
    Archive & ar, particules * t, const unsigned int file_version
){
    // retrieve data from archive required to construct new instance
    int nbParticules;
    ar >> nbParticules;
    // invoke inplace constructor to initialize instance of my_class
    ::new(t)particules(nbParticules);
}
}}


class ParticuleType : public BaseData
{
public:


    ParticuleType(std::shared_ptr<void> data = std::shared_ptr<void>()) : BaseData(data), p(NULL)
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
    virtual bool merge(char* serialdata, int size)
    {

        /*std::cout<<"Merging the data..."<<std::endl;
        FFSTypeHandle first_rec_handle;
        FFSContext fmc_r = create_FFSContext();
        first_rec_handle = FFSset_fixed_target(fmc_r, &particule_format_list[0]);


        buffer_ = serialdata;
        size_buffer_ = size;
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
        return true;*/
        return false;
    }

    virtual bool merge()
    {
        boost::iostreams::basic_array_source<char> device(in_serial_str.data(), in_size_buffer_);
        boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s(device);
        boost::archive::binary_iarchive ia(s);

        std::cout<<"Merging the data..."<<std::endl;

        std::cout<<"Serial buffer size : "<<in_size_buffer_<<";;"<<in_serial_str.size()<<std::endl;

        //We don't already have a data. Filling the structure
        if(!data_)
        {
            std::cout<<"No data available in the current object, create one"<<std::endl;

            //Allocating the necessary space
            //data_ = shared_ptr<void>(new char[sizeof(particules)],
            //        std::default_delete<char[]>());
            //p = (particules*)(data_.get());
            //std::cout<<"Test with a normal new object"<<std::endl;

            //Deserializing
            ia >> p;

            data_ = static_pointer_cast<void>(std::shared_ptr<particules>(p));

            //Updating the object infos
            nbItems_ = p->nbParticules;
            nbItems_ > 1 ? splitable_ = true: splitable_ = false;
        }
        else //We have to decode elsewhere since the data will be appended
             //to the current data buffer
        {
            std::cout<<"Data available in the current object, create one extra buffer"<<std::endl;
            particules* otherParticule;

            //Deserializing
            ia >> otherParticule;

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

            //Cleaning temporary data
            delete otherParticule;

            std::cout<<"The new data has "<<p->nbParticules<<" particules"<<std::endl;

            //Updating the info of the object
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

        boost::iostreams::back_insert_device<std::string> inserter(out_serial_str);
        boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
        boost::archive::binary_oarchive oa(s);

        oa << p;

        s.flush();
        out_size_buffer_ = out_serial_str.size();

        std::cout<<"Serialization successful (size : "<<out_size_buffer_<<")"<<std::endl;

        return true;
    }

    virtual bool unserialize()
    {
        return false;
    }

    virtual void allocate_serial_buffer(int size)
    {
        if(in_serial_str.size() < size)
            in_serial_str.resize(size);
        in_size_buffer_ = size;
    }

    /*virtual char* getSerialBuffer(int* size)
    {
        *size = size_buffer_;
        return &serial_str[0]; //Very bad
    }

    virtual char* getSerialBuffer(){ return &serial_str[0]; }
    virtual int getSerialBufferSize(){ return size_buffer_; }*/

    virtual char* getOutSerialBuffer(int* size)
    {
        *size = out_size_buffer_;
        return &out_serial_str[0]; //Very bad
    }
    virtual char* getOutSerialBuffer(){ return &out_serial_str[0]; }
    virtual int getOutSerialBufferSize(){ return out_size_buffer_; }

    virtual char* getInSerialBuffer(int* size)
    {
        *size = in_size_buffer_;
        return &in_serial_str[0]; //Very bad
    }
    virtual char* getInSerialBuffer(){ return &in_serial_str[0]; }
    virtual int getInSerialBufferSize(){ return in_size_buffer_; }


protected:

    std::string in_serial_str;
    std::string out_serial_str;
    int in_size_buffer_;
    int out_size_buffer_;
    particules *p;

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
            char* buffer = part->getOutSerialBuffer(&buffer_size);
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

        // Creating an empty object in which we will uncode the message
        ParticuleType *part = new ParticuleType();
        part->allocate_serial_buffer(nitems);

        MPI_Recv(part->getInSerialBuffer(), nitems,
               MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
        std::cout<<"Reception of a message of size "<<nitems<<std::endl;


        part->merge();

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
        component.flush();


    }
    else if(rank == 2)
    {
        std::shared_ptr<ParticuleType> result = std::shared_ptr<ParticuleType>(
                    new ParticuleType());
        component.process(result, decaf::DECAF_REDIST_DEST);
        component.flush();

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
        component.flush();


    }
    else if(rank < nbSource + nbReceptors)
    {
        std::shared_ptr<ParticuleType> result = std::shared_ptr<ParticuleType>(
                    new ParticuleType());
        component.process(result, decaf::DECAF_REDIST_DEST);
        component.flush();

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

int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);

    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //runTestSimple();
    runTestParallelRedist(5,2);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

  return 0;
}
