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

#include <manala/storage/storagecollectioninterface.hpp>
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
#endif
