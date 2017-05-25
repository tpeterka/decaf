#include <manala/storage/storagecollectioninterface.hpp>

decaf::
StorageCollectionInterface::~StorageCollectionInterface()
{
    for(Storage* storage : storages)
        delete storage;
}

bool
decaf::
StorageCollectionInterface::isFull()
{
    for(Storage* storage : storages)
    {
        if(!storage->isFull())
            return false;
    }

    return true;
}

unsigned int
decaf::
StorageCollectionInterface::getBufferSize(unsigned int index)
{
    if(index < storages.size())
        return storages[index]->getBufferSize();
    else
        return 0;
}

unsigned int
decaf::
StorageCollectionInterface::getNbStorageObjects()
{
    return storages.size();
}

void
decaf::
StorageCollectionInterface::addBuffer(Storage *storage)
{
    storages.push_back(storage);
}

bool
decaf::
StorageCollectionInterface::hasData(unsigned int id)
{
    for(Storage* storage : storages)
    {
        if(storage->hasData(id))
            return true;
    }

    return false;
}

pConstructData
decaf::
StorageCollectionInterface::getData(unsigned int id)
{
    for(Storage* storage : storages)
    {
        if(storage->hasData(id))
            return storage->getData(id);
    }

    fprintf(stderr,"ERROR: Data not found in the storage.\n");
    return pConstructData();
}

void
decaf::
StorageCollectionInterface::save(unsigned int rank)
{
    vector<pair<unsigned int, string> > filelist;

    StorageFile storagefile(1000000, rank);

    // First we store all the data
    for(Storage* storage : storages)
    {
        // Checking if the current storage is already a FileStorage
        StorageFile* file = dynamic_cast<StorageFile*>(storage);
        if(file)
        {
            fprintf(stderr,"The current storage use files.\n");
            file->fillFilelist(filelist);
        }
        else
            fprintf(stderr, "The current storage is not using file. Retrieving data to store them.\n");

        unsigned int nb_data = storage->getNbDataStored();
        for(unsigned int index = 0; index < nb_data; index++)
        {
            // Retrieving the data informations
            unsigned int id = storage->getID(index);
            pConstructData data = storage->getData(id);

            //Saving into the file storage
            if(!storagefile.insert(id, data))
                fprintf(stderr, "ERROR: unable to storage the data id %u on rank %i\n", id, rank);
        }
    }

    // All the data are now on file. Retrieving the informations
    storagefile.fillFilelist(filelist);

    // Building the metadata file
    const char* folder_prefix = getenv("DECAF_STORAGE_FOLDER");
    string folder_name;
    if(folder_prefix != nullptr)
        folder_name = string(folder_prefix);
    else
        folder_name = string("/tmp");
    stringstream filename;
    filename<<folder_name<<"/filelist_"<<rank<<".txt";

    struct stat stats;
    if(stat(filename.str().c_str(), &stats) == 0)
    {
        fprintf(stderr,"ERROR: File already exist, unable to write the metadata.\n");
        return;
    }

    std::ofstream file(filename.str());

    // Writing the list of files
    for(auto v : filelist)
    {
        file << v.first<<" "<<v.second<<endl;
    }
    file.close();
}

unsigned int
decaf::
StorageCollectionInterface::getNbDataStored()
{
    unsigned int result = 0;
    for(Storage* storage : storages)
        result += storage->getNbDataStored();
    return result;
}
