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

#include <manala/storage/storageinterface.hpp>
#include <manala/storage/storagefile.hpp>
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
        unsigned int getNbDataStored();

    protected:
        std::vector<Storage*> storages;
    };
}

#endif
