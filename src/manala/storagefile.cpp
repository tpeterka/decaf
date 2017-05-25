#include <manala/storage/storagefile.hpp>

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

unsigned int
decaf::
StorageFile::getID(unsigned int index)
{
    auto it = buffer_.begin();
    for(unsigned int i = 0; i < index; i++)
        it++;
    return (*it).first;
}

unsigned int
decaf::
StorageFile::getNbDataStored()
{
    return buffer_.size();
}

string
decaf::StorageFile::getFilename(unsigned int id)
{
    auto it = buffer_.find(id);
    if(it == buffer_.end())
        return string("");
    else
        return (*it).second;
}

void
decaf::
StorageFile::fillFilelist(vector<pair<unsigned int, string> >& filelist)
{
    for(auto it : buffer_)
    {
        filelist.emplace_back(it.first, it.second);
    }
}

void
decaf::
StorageFile::initFromFilelist(vector<pair<unsigned int, string> >& filelist)
{
    for(auto it : filelist)
    {
        auto result = buffer_.insert(it);
        if(!result.second)
            fprintf(stderr, "ERROR: Unable to load the file id %i in the StorageFile.\n", it.first);

    }
}
