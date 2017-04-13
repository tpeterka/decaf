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

#ifndef DECAF_STORAGE_BINARYFILE
#define DECAF_STORAGE_BINARYFILE

#include <decaf/data_model/pconstructtype.h>
#include <decaf/storage/storageinterface.hpp>
#include <map>
#include <set>
#include <sys/stat.h>
#include <fstream>

namespace decaf
{
    class StorageFile: public Storage
    {
    public:
        StorageFile(unsigned int buffer_max_size, unsigned int rank);

        virtual ~StorageFile(){}

        virtual bool isFull();
        virtual unsigned int getBufferSize();
        virtual bool insert(unsigned int id, pConstructData data );
        virtual void erase(unsigned int id);
        virtual bool hasData(unsigned int id);
        virtual pConstructData getData(unsigned int id);
        virtual void processCommand(FrameCommand command, unsigned int frame_id);

    protected:
        unsigned int rank_;
        unsigned int buffer_max_size_;
        std::map<unsigned int, string> buffer_;
        string folder_name_;
    };
} // namespace decaf

decaf::
StorageFile::StorageFile(unsigned int buffer_max_size, unsigned int rank) :
    Storage(), buffer_max_size_(buffer_max_size), rank_(rank)
{
    const char* folder_prefix = getenv("DECAF_STORAGE_FOLDER");
    if(folder_prefix != nullptr)
        folder_name_ = string(folder_prefix);
    else
        folder_name_ = string("/tmp");
}

bool
decaf::
StorageFile::isFull()
{
    return buffer_.size() >= buffer_max_size_;
}

unsigned int
decaf::
StorageFile::getBufferSize()
{
    return buffer_.size();
}

bool
decaf::
StorageFile::insert(unsigned int id, pConstructData data)
{
    if(buffer_.size() >= buffer_max_size_)
        return false;

    //auto it = buffer_.insert(std::pair<unsigned int, pConstructData>(id, data));
    //Creation of the filename
    stringstream filename;
    filename<<folder_name_<<"/save_"<<rank_<<"_"<<id;

    struct stat stats;
    if(stat(filename.str().c_str(), &stats) == 0)
    {
        fprintf(stderr,"ERROR: File already exist, unable to buffer a data.\n");
        return false;
    }

    buffer_.insert(make_pair(id, filename.str()));

    std::ofstream file(filename.str(), std::ofstream::binary);

    data->serialize();
    file.write(data->getOutSerialBuffer(), data->getOutSerialBufferSize());

    file.close();

    return true;
}

void
decaf::
StorageFile::erase(unsigned int id)
{
    auto it = buffer_.find(id);
    if(it == buffer_.end())
        return;

    remove(it->second.c_str());
    buffer_.erase(it);

    return;
}

bool
decaf::
StorageFile::hasData(unsigned int id)
{
    return buffer_.count(id) > 0;
}

decaf::pConstructData
decaf::
StorageFile::getData(unsigned int id)
{
    pConstructData container;

    // Getting the filename associated to the id
    auto it = buffer_.find(id);
    if(it == buffer_.end())
    {
        fprintf(stderr,"ERROR: no associated filename for the ID %u\n", id);
        return container;
    }

    // Initializing the file descriptor
    ifstream file(it->second, std::ifstream::binary);
    if(!file.good())
    {
        fprintf(stderr,"ERROR: unable to read the file %s for ID %u\n", it->second.c_str(), id);
        return container;
    }

    // Get the size of the file
    file.seekg (0,file.end);
    long size = file.tellg();
    file.seekg (0);

    // Write the buffer into the container
    container->allocate_serial_buffer(size);
    file.read(container->getInSerialBuffer(), size);

    // Unserialize the file into the container
    container->merge();

    return container;

}

void
decaf::
StorageFile::processCommand(FrameCommand command, unsigned int frame_id)
{
    switch(command)
    {
        case DECAF_FRAME_COMMAND_REMOVE:
        {
            erase(frame_id);
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL:
        {
            auto it = buffer_.begin();
            while(it != buffer_.end() && it->first <= frame_id)
            {
                erase(it->first);
                it++;
            }
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL_EXCLUDED:
        {
            auto it = buffer_.begin();
            while(it != buffer_.end() && it->first < frame_id)
            {
                erase(it->first);
                it++;
            }
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: unknown FrameCommand in storagemainmemory.\n");
            break;
        }
    }
}




#endif