//---------------------------------------------------------------------------
//
// Example of serializations with Boost and manipulation with the data model
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/vectorconstructdata.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/boost_macros.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <bredala/transport/mpi/redist_count_mpi.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <mpi.h>
#include <sstream>

using namespace decaf;
using namespace std;

class particles
{
public:
    int nbparticles;
    int sizeArray;
    float *x;
    float *v;

    particles(int nb = 0) //To test if we can switch array for vector with FFS
    {
        nbparticles = nb;
        sizeArray = nb * 3;
        if(nbparticles > 0){
            x = (float*)malloc(sizeArray * sizeof(float));
            v = (float*)malloc(sizeArray * sizeof(float));
        }
    }

    particles(int nb, float* x_, float* v_)
    {
        nbparticles = nb;
        sizeArray = nb * 3;
        x = x_;
        v = v_;
    }

    ~particles()
    {
        if(sizeArray > 0){
            free(x);
            free(v);
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & nbparticles;
        ar & sizeArray;
        ar & boost::serialization::make_array<float>(x,sizeArray);
        ar & boost::serialization::make_array<float>(v,sizeArray);
    }

};

namespace boost { namespace serialization {
template<class Archive>
inline void save_construct_data(
    Archive & ar, const particles * t, const unsigned int file_version
){
    // save data required to construct instance
    ar << t->nbparticles;
}

template<class Archive>
inline void load_construct_data(
    Archive & ar, particles * t, const unsigned int file_version
){
    // retrieve data from archive required to construct new instance
    int nbparticles;
    ar >> nbparticles;
    // invoke inplace constructor to initialize instance of my_class
    ::new(t)particles(nbparticles);
}
}}

void simpleSerializeMPI()
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < 2){
        std::cout<<"Not enough ranks in the world com. 2 needed"<<std::endl;
        return;
    }

    if(rank == 0)
    {
        particles *p = new particles(10);
        for(unsigned int i = 0; i < p->sizeArray; i++)
        {
            p->v[i] = 1.0 * (float)(i+1);
            p->x[i] = 3.0 * (float)(i+1);
        }

        std::string serial_str;

        boost::iostreams::back_insert_device<std::string> inserter(serial_str);
        boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
        boost::archive::binary_oarchive oa(s);

        oa << p;

        s.flush();

        std::cout<<"Serialization successful (size : "<<serial_str.size()<<")"<<std::endl;

        MPI_Send(serial_str.data(), serial_str.size(), MPI_BYTE, 1,0,MPI_COMM_WORLD);
        delete p;
    }
    else if(rank == 1)
    {
        MPI_Status status;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int nitems;
        MPI_Get_count(&status, MPI_BYTE, &nitems);

        std::string serial_str;
        serial_str.resize(nitems);
        std::cout<<"Size of the string : "<<serial_str.size()<<std::endl;
        MPI_Recv(&serial_str[0], nitems, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG,
                MPI_COMM_WORLD, &status);

        boost::iostreams::basic_array_source<char> device(serial_str.data(), serial_str.size());
        boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
        boost::archive::binary_iarchive ia(sout);

        particles * newparticle;
        ia >> newparticle;
        std::cout<<"Le nouvel object a "<<newparticle->nbparticles<<" particles"<<std::endl;
        std::cout<<"Test with normal object successful"<<std::endl;

        delete newparticle;
    }

}

void simpleSerializeTest()
{
    particles *p = new particles(10);
    for(unsigned int i = 0; i < p->sizeArray; i++)
    {
        p->v[i] = 1.0 * (float)(i+1);
        p->x[i] = 3.0 * (float)(i+1);
    }

    std::string serial_str;

    boost::iostreams::back_insert_device<std::string> inserter(serial_str);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
    boost::archive::binary_oarchive oa(s);

    oa << p;

    s.flush();

    std::cout<<"Serialization successful (size : "<<serial_str.size()<<")"<<std::endl;

    boost::iostreams::basic_array_source<char> device(serial_str.data(), serial_str.size());
    boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
    boost::archive::binary_iarchive ia(sout);

    particles * newparticle;
    ia >> newparticle;
    std::cout<<"Le nouvel object a "<<newparticle->nbparticles<<" particles"<<std::endl;
    std::cout<<"Test with normal object successful"<<std::endl;


    delete p;
    delete newparticle;
}


void testConstructType()
{
    std::cout<<"Test of the serialization fonctionnality with ContructType"<<std::endl;

    std::vector<float> pos{0.0,1.0,2.0,3.0,4.0,5.0};
    int nbparticle = pos.size() / 3;

    std::shared_ptr<VectorConstructData<float> > array = std::make_shared<VectorConstructData<float> >( pos, 3 );
    std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbparticle );

    ConstructData container;
    container.appendData(std::string("nbparticles"), data);
    container.appendData(std::string("pos"), array);

    /*std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);

    oa << container;

    ss.flush();

    std::cout<<"Serialization successful (size : "<<ss.str().size()<<")"<<std::endl;

    boost::archive::binary_iarchive ia(ss);

    ConstructData newContainer;
    ia >> newContainer;*/

    if(!container.serialize())
    {
        std::cout<<"ERROR : unable to serialize the map"<<std::endl;
        return;
    }

    /*boost::archive::binary_iarchive ia(container.serial_buffer_);

    std::shared_ptr<std::map<std::string, datafield> > newContainer;
    ia >> newContainer;
    std::cout<<"The new map has "<<newContainer->size()<<" datafield"<<std::endl;*/

    int size_buffer;
    char *serial_buffer = container.getOutSerialBuffer(&size_buffer);
    std::cout<<"Serialization succeeded. Size of serial buffer : "<<size_buffer<<std::endl;

    ConstructData newContainer;
    //newContainer.merge(serial_buffer, size_buffer);
    newContainer.allocate_serial_buffer(size_buffer);
    memcpy(newContainer.getInSerialBuffer(), serial_buffer, size_buffer);
    newContainer.merge();

    std::cout<<"The new map has "<<newContainer.getNbItems()<<" datafield"<<std::endl;
    std::cout<<newContainer.getMap()->size()<<std::endl;
    newContainer.printKeys();
}

void printMap(pConstructData map)
{
    std::shared_ptr<VectorConstructData<float> > array = dynamic_pointer_cast<VectorConstructData<float> >(map->getData("pos"));
    std::shared_ptr<SimpleConstructData<int> > data = dynamic_pointer_cast<SimpleConstructData<int> >(map->getData("nbparticles"));

    std::cout<<"Number of particle : "<<data->getData()<<std::endl;
    std::cout<<"Positions : [";
    for(unsigned int i = 0; i < array->getVector().size(); i++)
        std::cout<<array->getVector().at(i)<<",";
    std::cout<<"]"<<std::endl;
}

/*void testConstructTypeSplit()
{
    std::cout<<"Test of the serialization fonctionnality with ContructType"<<std::endl;
    std::cout<<"SPLITTING BASE OBJECT"<<std::endl;
    std::vector<float> pos{0.0,1.0,2.0,3.0,4.0,5.0,0.0,1.0,2.0,3.0,4.0,5.0};
    int nbparticle = pos.size() / 3;

    std::shared_ptr<VectorConstructData<float> > array = std::make_shared<VectorConstructData<float> >( pos, 3 );
    std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbparticle );

    pConstructData container;
    container->appendData(std::string("nbparticles"), data,
                         DECAF_NOFLAG, DECAF_SHARED,
                         DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
    container->appendData(std::string("pos"), array,
                         DECAF_NOFLAG, DECAF_PRIVATE,
                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    std::vector<int> ranges {1,2,1};
    std::vector<std::shared_ptr<BaseData> > partialContainers = container->split(ranges);
    std::cout<<"----------------------------"<<std::endl
             <<"Number of splits containers : "<<partialContainers.size()<<std::endl;
    for(unsigned int i = 0; i < partialContainers.size(); i++)
    {
        std::cout<<"==========================="<<std::endl;
        std::shared_ptr<ConstructData> newContainer = dynamic_pointer_cast<ConstructData>(partialContainers.at(i));
        std::cout<<"Map "<<i<<" has "<<newContainer->getNbItems()<<" items."<<std::endl;
        std::cout<<"Map "<<i<<" has "<<newContainer->getMap()->size()<<" fields."<<std::endl;
        newContainer->printKeys();
        printMap(pConstructData(newContainer));
        std::cout<<"==========================="<<std::endl;
    }
    std::cout<<"----------------------------"<<std::endl;

    std::cout<<"MERGING SPLITTED OBJECTS"<<std::endl;
    pConstructData mergeContainer;
    for(unsigned int i = 0; i < partialContainers.size(); i++)
    {
        if(!mergeContainer->merge(partialContainers.at(i)))
            std::cout<<"ERROR : merging failed"<<std::endl;
        printMap(mergeContainer);
    }
    std::cout<<"==========================="<<std::endl;
    std::cout<<"Merged map has "<<mergeContainer->getNbItems()<<" items."<<std::endl;
    std::cout<<"Merged map has "<<mergeContainer->getMap()->size()<<" fields."<<std::endl;
    mergeContainer->printKeys();
    std::cout<<"==========================="<<std::endl;
}*/

void testConstructTypeSplitMPI()
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < 4){
        std::cout<<"ERROR : Not enough rank available for testConstructTypeSplitMPI. Minimum 4"<<std::endl;
        return;
    }

    if(rank >= 4)
        return;

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Complete Test with Redistribution component (1 split, 1 merge)"<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(rank == 0)
    {
        std::cout<<"Test of the serialization fonctionnality with ContructType and MPI"<<std::endl;
        std::cout<<"SPLITTING BASE OBJECT"<<std::endl;
        std::vector<float> pos{0.0,1.0,2.0,3.0,4.0,5.0,0.0,1.0,2.0,3.0,4.0,5.0};
        int nbparticle = pos.size() / 3;

        std::shared_ptr<VectorConstructData<float> > array = std::make_shared<VectorConstructData<float> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbparticle );

        pConstructData container;
        container->appendData(std::string("nbparticles"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container->appendData(std::string("pos"), array,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        std::vector<int> ranges {1,2,1};
        std::vector<std::shared_ptr<BaseData> > partialContainers = container->split(ranges);
        std::cout<<"----------------------------"<<std::endl
                 <<"Number of splits containers : "<<partialContainers.size()<<std::endl;
        for(unsigned int i = 0; i < partialContainers.size(); i++)
        {
            std::cout<<"==========================="<<std::endl;
            std::shared_ptr<ConstructData> newContainer = dynamic_pointer_cast<ConstructData>(partialContainers.at(i));
            std::cout<<"Map "<<i<<" has "<<newContainer->getNbItems()<<" items."<<std::endl;
            std::cout<<"Map "<<i<<" has "<<newContainer->getMap()->size()<<" fields."<<std::endl;
            newContainer->printKeys();
            printMap(pConstructData(newContainer));
            std::cout<<"==========================="<<std::endl;
        }
        std::cout<<"----------------------------"<<std::endl;

        for(int i = 0; i < partialContainers.size(); i++)
        {
            if(partialContainers.at(i)->serialize())
            {
                MPI_Send(partialContainers.at(i)->getOutSerialBuffer(),
                         partialContainers.at(i)->getOutSerialBufferSize(),
                         MPI_BYTE, i+1, 0, MPI_COMM_WORLD);
            }
            else
                std::cout<<"ERROR : Unable to serialized a data. No data sent to "<<i<<std::endl;
        }

        std::cout<<"Sending of the splitted containers completed."<<std::endl;

    }
    else if(rank > 0 && rank < 4)
    {

        std::cout<<"Rank "<<rank<<" Receiving data"<<std::endl;
        pConstructData container;
        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == 0)  // normal, non-null get
        {
            int nitems; // number of items (of type dtype) in the message
            MPI_Get_count(&status, MPI_BYTE, &nitems);

            //Allocating the space necessary
            container->allocate_serial_buffer(nitems);

            MPI_Recv(container->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                     status.MPI_TAG, MPI_COMM_WORLD, &status);
            container->merge();
        }
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Merged map has "<<container->getNbItems()<<" items."<<std::endl;
        std::cout<<"Merged map has "<<container->getMap()->size()<<" fields."<<std::endl;
        container->printKeys();
        printMap(container);
        std::cout<<"==========================="<<std::endl;

        //Sending back the data for gathering
        //No need to serialize again, we haven't touch the serial buffer
        // NOTE : Normally we use getOutSerialBuffer() to get the serial buffer
        // But here the container only forward data without deserialize/reserialize the data
        MPI_Send(container->getInSerialBuffer(), container->getInSerialBufferSize(),
                 MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        std::cout<<"Sending back the splitted containers completed."<<std::endl;
    }

    if(rank == 0)
    {
        std::cout<<"Receiving the data after splitting. Merging..."<<std::endl;
        pConstructData container;
        for(int i = 0; i < 3; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == 0)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                //Allocating the space necessary
                container->allocate_serial_buffer(nitems);

                MPI_Recv(container->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                         status.MPI_TAG, MPI_COMM_WORLD, &status);
                container->merge();
            }
        }
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<container->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<container->getMap()->size()<<" fields."<<std::endl;
        container->printKeys();
        printMap(container);
        std::cout<<"==========================="<<std::endl;

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

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(rank < nbSource){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        std::vector<float> pos{0.0,1.0,2.0,3.0,4.0,5.0,0.0,1.0,2.0};
        int nbparticle = pos.size() / 3;
        std::shared_ptr<VectorConstructData<float> > array = std::make_shared<VectorConstructData<float> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbparticle );


        pConstructData container;
        container->appendData(std::string("nbparticles"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container->appendData(std::string("pos"), array,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        component.process(container, decaf::DECAF_REDIST_SOURCE);
    }
    else if(rank < nbSource + nbReceptors)
    {
        pConstructData result;
        component.process(result, decaf::DECAF_REDIST_DEST);

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        result->printKeys();
        printMap(result);
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors<<" consummer completed"<<std::endl;
    }

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component completed"<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;
}

void runTestParallelRedistOverlap(int startSource, int nbSource, int startReceptors, int nbReceptors)
{
    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size_world < max(startSource + nbSource, startReceptors + nbReceptors))
    {
        std::cout<<"ERROR : not enough rank for "<<nbSource<<" sources and "<<nbReceptors<<" receivers"<<std::endl;
        return;
    }

    if(rank >= max(startSource + nbSource, startReceptors + nbReceptors))
        return;

    RedistCountMPI *component = new RedistCountMPI(startSource, nbSource, startReceptors, nbReceptors, MPI_COMM_WORLD);

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping..."<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;

    if(rank >= startSource && rank < startSource + nbSource){
        std::cout<<"Running Redistributed test between "<<nbSource<<" producers"
                   "and "<<nbReceptors<<" consummers"<<std::endl;

        /*std::vector<float> pos{0.0,1.0,2.0,3.0,4.0,5.0,0.0,1.0,2.0};
        int nbparticle = pos.size() / 3;
        std::shared_ptr<VectorConstructData<float> > array = std::make_shared<VectorConstructData<float> >( pos, 3 );
        std::shared_ptr<SimpleConstructData<int> > data  = std::make_shared<SimpleConstructData<int> >( nbparticle );


        std::shared_ptr<BaseData> container = std::shared_ptr<ConstructData>(new ConstructData());
        std::shared_ptr<ConstructData> object = dynamic_pointer_cast<ConstructData>(container);
        object->appendData(std::string("nbparticles"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        object->appendData(std::string("pos"), array,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);*/

        std::shared_ptr<SimpleConstructData<int> > data  =
                std::make_shared<SimpleConstructData<int> >( 1 );

        pConstructData container;
        container->appendData(std::string("t_current"), data,
                             DECAF_NOFLAG, DECAF_PRIVATE,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        component->process(container, decaf::DECAF_REDIST_SOURCE);
        component->flush();
    }
    if(rank >= startReceptors && rank < startReceptors + nbReceptors)
    {
        pConstructData result;
        component->process(result, decaf::DECAF_REDIST_DEST);
        component->flush();

        std::cout<<"==========================="<<std::endl;
        std::cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<std::endl;
        std::cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<std::endl;
        result->printKeys();
        //printMap(*result);
        std::cout<<"==========================="<<std::endl;
        std::cout<<"Simple test between "<<nbSource<<" producers and "<<nbReceptors<<" consummer completed"<<std::endl;
    }

    delete component;

    std::cout<<"-------------------------------------"<<std::endl;
    std::cout<<"Test with Redistribution component with overlapping completed"<<std::endl;
    std::cout<<"-------------------------------------"<<std::endl;
}





int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);

    int size_world, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 0)
    {
        simpleSerializeTest();
        testConstructType();
        //testConstructTypeSplit();
    }

    MPI_Barrier(MPI_COMM_WORLD);
    testConstructTypeSplitMPI();
    MPI_Barrier(MPI_COMM_WORLD);
    runTestParallelRedist(3,2);
    MPI_Barrier(MPI_COMM_WORLD);
    runTestParallelRedistOverlap(0, 4, 1, 2);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
