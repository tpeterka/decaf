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

#include <decaf/data_model/pconstructtype.h>
#include <decaf/storage/storageinterface.hpp>
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

    protected:
        unsigned int buffer_max_size_;
        std::map<unsigned int, pConstructData> buffer_;
    };
} // namespace decaf

bool
decaf::
StorageMainMemory::isFull()
{
    return buffer_.size() >= buffer_max_size_;
}

unsigned int
decaf::
StorageMainMemory::getBufferSize()
{
    return buffer_.size();
}

bool
decaf::
StorageMainMemory::insert(unsigned int id, pConstructData data)
{
    if(buffer_.size() >= buffer_max_size_)
        return false;

    auto it = buffer_.insert(std::pair<unsigned int, pConstructData>(id, data));

    return it.second;
}

void
decaf::
StorageMainMemory::erase(unsigned int id)
{
    buffer_.erase(id);
}

bool
decaf::
StorageMainMemory::hasData(unsigned int id)
{
    return buffer_.count(id) > 0;
}

decaf::pConstructData
decaf::
StorageMainMemory::getData(unsigned int id)
{
    return buffer_.at(id);
}

void
decaf::
StorageMainMemory::processCommand(FrameCommand command, unsigned int frame_id)
{
    switch(command)
    {
        case DECAF_FRAME_REMOVE:
        {
            buffer_.erase(frame_id);
            break;
        }
        case DECAF_FRAME_REMOVE_UNTIL:
        {
            auto it = buffer_.begin();
            while(it != buffer_.end() && it->first <= frame_id)
                buffer_.erase(it++);
            break;
        }
        case DECAF_FRAME_REMOVE_UNTIL_EXCLUDED:
        {
            auto it = buffer_.begin();
            while(it != buffer_.end() && it->first < frame_id)
                buffer_.erase(it++);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: unknown FrameCommand.\n");
            break;
        }
    }
}


#endif
