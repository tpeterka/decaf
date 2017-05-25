#include <manala/storage/storagemainmemory.hpp>

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
        case DECAF_FRAME_COMMAND_REMOVE:
        {
            buffer_.erase(frame_id);
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL:
        {
            auto it = buffer_.begin();
            while(it != buffer_.end() && it->first <= frame_id)
            {
                buffer_.erase(it++);
            }
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL_EXCLUDED:
        {
            auto it = buffer_.begin();
            while(it != buffer_.end() && it->first < frame_id)
            {
                buffer_.erase(it++);
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
StorageMainMemory::getID(unsigned int index)
{
    auto it = buffer_.begin();
    for(unsigned int i = 0; i < index; i++)
        it++;
    return (*it).first;
}

unsigned int
decaf::
StorageMainMemory::getNbDataStored()
{
    return buffer_.size();
}
