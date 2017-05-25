#include <manala/storage/storagecollectiongreedy.hpp>

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

void
decaf::
StorageCollectionGreedy::processCommand(FrameCommand command, unsigned int frame_id)
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
            break;
        }
        case DECAF_FRAME_COMMAND_REMOVE_UNTIL_EXCLUDED:
        {
            for(Storage* storage : storages)
                storage->processCommand(command, frame_id);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: unknown FrameCommand.\n");
            break;
        }
    };
}
