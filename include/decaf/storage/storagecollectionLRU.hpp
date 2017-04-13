//---------------------------------------------------------------------------
// Define the a storage collection adopting a LRU strategy.
// A new data is added into the first storage with enough space available
// If no space is available, remove the oldest data and replaced it
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#ifndef DECAF_STORAGE_COLLECTION_LRU_HPP
#define DECAF_STORAGE_COLLECTION_LRU_HPP

#include <decaf/storage/storagecollectioninterface.hpp>
#include <deque>


namespace decaf
{
    class StorageCollectionLRU : public StorageCollectionInterface
    {
    public:
        StorageCollectionLRU(){}

        virtual ~StorageCollectionLRU(){}

        virtual bool isFull();
        virtual bool insert(unsigned int id, pConstructData data);
        virtual void erase(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id);

    protected:
        // Keep the history of the content
        deque<pair<unsigned int, Storage*> > index;
    };
}

bool
decaf::
StorageCollectionLRU::isFull()
{
    return false;
}

bool
decaf::
StorageCollectionLRU::insert(unsigned int id, pConstructData data)
{
    // Checking if we have room for the data
    for(Storage* storage : storages)
    {
        if(!storage->isFull())
        {
            if(storage->insert(id, data))
            {
                index.emplace_front(id, storage);
                return true;
            }
            else
            {
                fprintf(stderr,"ERROR: insertion failed in the selected storage.\n");
                return false;
            }
        }
    }

    // All the storages are full. Removing the oldest element.
    Storage* storage = index.back().second;
    storage->erase(index.back().first);
    index.pop_back();

    // Inserting the new data where we made space.
    if(!storage->insert(id, data))
    {
        fprintf(stderr,"ERROR: insertion failed in the selected storage.\n");
        return false;
    }
    else
        index.emplace_front(id, storage);

    return true;
}

void
decaf::
StorageCollectionLRU::erase(unsigned int id)
{
    for(Storage* storage : storages)
    {
        storage->erase(id);
    }

    assert(id == index.back().first);
    index.pop_back();
}

void
decaf::
StorageCollectionLRU::processCommand(FrameCommand command, unsigned int frame_id)
{
    switch(command)
    {
        case DECAF_FRAME_COMMAND_REMOVE:
        {
            this->erase(frame_id);
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL:
        {
            for(Storage* storage : storages)
                storage->processCommand(command, frame_id);
            while(!index.empty() && index.back().first <= frame_id)
                index.pop_back();
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL_EXCLUDED:
        {
            for(Storage* storage : storages)
                storage->processCommand(command, frame_id);
            while(!index.empty() && index.back().first < frame_id)
                index.pop_back();
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: unknown FrameCommand.\n");
            break;
        }
    };
}

#endif
