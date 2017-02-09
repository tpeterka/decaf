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


#ifndef DECAF_STORAGE_COLLECTION_INTERFACE
#define DECAF_STORAGE_COLLECTION_INTERFACE

#include <decaf/storage/storageinterface.hpp>
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

        bool isFull();
        unsigned int getBufferSize(unsigned int index);
        unsigned int getNbStorageObjects();
        virtual bool insert(unsigned int id, pConstructData data) = 0;
        virtual void erase(unsigned int id) = 0;
        virtual bool hasData(unsigned int id);
        virtual pConstructData getData(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id) = 0;
        void addBuffer(Storage* storage);

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
}

#endif
