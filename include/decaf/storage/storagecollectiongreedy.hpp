//---------------------------------------------------------------------------
// Define the a storage collection adopting a greedy strategy.
// A new data is added into the first storage with enough space available
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#ifndef DECAF_STORAGE_COLLECTION_GREEDY
#define DECAF_STORAGE_COLLECTION_GREEDY

#include <decaf/storage/storagecollectioninterface.hpp>

namespace decaf
{
    class StorageCollectionGreedy : public StorageCollectionInterface
    {
    public:
        StorageCollectionGreedy(){}

        virtual ~StorageCollectionGreedy(){}

        virtual bool insert(unsigned int id, pConstructData data);
        virtual void erase(unsigned int id);
    };
}

bool
decaf::
StorageCollectionGreedy::insert(unsigned int id, pConstructData data)
{
    for(Storage* storage : storages)
    {
        if(!storage->isFull())
            return storage->insert(id, data);
    }

    fprintf(stderr,"ERROR: the collection is full.\n");
    return false;
}

void
decaf::
StorageCollectionGreedy::erase(unsigned int id)
{
    for(Storage* storage : storages)
    {
        storage->erase(id);
    }
}


#endif
