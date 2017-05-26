//---------------------------------------------------------------------------
// Implement the storage interface for local memory. This storage object
// store data in a local map accessible by the local process.
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_STORAGE_MAINMEMORY
#define DECAF_STORAGE_MAINMEMORY

#include <bredala/data_model/pconstructtype.h>
#include <manala/storage/storageinterface.hpp>
#include <map>
#include <set>

namespace decaf
{
    class StorageMainMemory: public Storage
    {
    public:
        StorageMainMemory(unsigned int buffer_max_size) :
            Storage(), buffer_max_size_(buffer_max_size){}

        virtual ~StorageMainMemory(){}

        virtual bool isFull();
        virtual unsigned int getBufferSize();
        virtual bool insert(unsigned int id, pConstructData data );
        virtual void erase(unsigned int id);
        virtual bool hasData(unsigned int id);
        virtual pConstructData getData(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id);
        virtual unsigned int getID(unsigned int index);
        virtual unsigned int getNbDataStored();


    protected:
        unsigned int buffer_max_size_;
        std::map<unsigned int, pConstructData> buffer_;
    };
} // namespace decaf

#endif
