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


#ifndef DECAF_STORAGE_COLLECTION_GREEDY_HPP
#define DECAF_STORAGE_COLLECTION_GREEDY_HPP

#include <manala/storage/storagecollectioninterface.hpp>

namespace decaf
{
    class StorageCollectionGreedy : public StorageCollectionInterface
    {
    public:
        StorageCollectionGreedy(){}

        virtual ~StorageCollectionGreedy(){}

        virtual bool insert(unsigned int id, pConstructData data);
        virtual void erase(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id);
    };
}

#endif
