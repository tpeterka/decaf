//---------------------------------------------------------------------------
// Define the interface of a storage collection
// The interface can be used to implement different strategies
// to place the data into various storages
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#ifndef DECAF_STORAGE_COLLECTION_INTERFACE_HPP
#define DECAF_STORAGE_COLLECTION_INTERFACE_HPP

#include <decaf/storage/storageinterface.hpp>
#include <decaf/storage/storagefile.hpp>
#include <decaf/types.hpp>
#include <vector>
using namespace decaf;

namespace decaf
{
    class StorageCollectionInterface
    {
    public:
        StorageCollectionInterface(){}

        virtual ~StorageCollectionInterface();

        virtual bool isFull();
        unsigned int getBufferSize(unsigned int index);
        unsigned int getNbStorageObjects();
        virtual bool insert(unsigned int id, pConstructData data) = 0;
        virtual void erase(unsigned int id) = 0;
        virtual bool hasData(unsigned int id);
        virtual pConstructData getData(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id) = 0;
        void addBuffer(Storage* storage);
        void save(unsigned int rank);

    protected:
        std::vector<Storage*> storages;
    };
}

decaf::
StorageCollectionInterface::~StorageCollectionInterface()
{
    for(Storage* storage : storages)
        delete storage;
}

bool
decaf::
StorageCollectionInterface::isFull()
{
    for(Storage* storage : storages)
    {
        if(!storage->isFull())
            return false;
    }

    return true;
}

unsigned int
decaf::
StorageCollectionInterface::getBufferSize(unsigned int index)
{
    if(index < storages.size())
        return storages[index]->getBufferSize();
    else
        return 0;
}

unsigned int
decaf::
StorageCollectionInterface::getNbStorageObjects()
{
    return storages.size();
}

void
decaf::
StorageCollectionInterface::addBuffer(Storage *storage)
{
    storages.push_back(storage);
}

bool
decaf::
StorageCollectionInterface::hasData(unsigned int id)
{
    for(Storage* storage : storages)
    {
        if(storage->hasData(id))
            return true;
    }

    return false;
}

pConstructData
decaf::
StorageCollectionInterface::getData(unsigned int id)
{
    for(Storage* storage : storages)
    {
        if(storage->hasData(id))
            return storage->getData(id);
    }

    fprintf(stderr,"ERROR: Data not found in the storage.\n");
    return pConstructData();
}

void
decaf::
StorageCollectionInterface::save(unsigned int rank)
{
    vector<pair<unsigned int, string> > filelist;

    StorageFile storagefile(1000000, rank);

    // First we store all the data
    for(Storage* storage : storages)
    {
        // Checking if the current storage is already a FileStorage
        StorageFile* file = dynamic_cast<StorageFile*>(storage);
        if(file)
        {
            fprintf(stderr,"The current storage use files.\n");
            file->fillFilelist(filelist);
        }
        else
            fprintf(stderr, "The current storage is not using file. Retrieving data to store them.\n");

        unsigned int nb_data = storage->getNbDataStored();
        for(unsigned int index = 0; index < nb_data; index++)
        {
            // Retrieving the data informations
            unsigned int id = storage->getID(index);
            pConstructData data = storage->getData(id);

            //Saving into the file storage
            if(!storagefile.insert(id, data))
                fprintf(stderr, "ERROR: unable to storage the data id %u on rank %i\n", id, rank);
        }
    }

    // All the data are now on file. Retrieving the informations
    storagefile.fillFilelist(filelist);

    // Building the metadata file
    const char* folder_prefix = getenv("DECAF_STORAGE_FOLDER");
    string folder_name;
    if(folder_prefix != nullptr)
        folder_name = string(folder_prefix);
    else
        folder_name = string("/tmp");
    stringstream filename;
    filename<<folder_name<<"/filelist_"<<rank<<".txt";

    struct stat stats;
    if(stat(filename.str().c_str(), &stats) == 0)
    {
        fprintf(stderr,"ERROR: File already exist, unable to write the metadata.\n");
        return;
    }

    std::ofstream file(filename.str());

    // Writing the list of files
    for(auto v : filelist)
    {
        file << v.first<<" "<<v.second<<endl;
    }
    file.close();
}

#endif
