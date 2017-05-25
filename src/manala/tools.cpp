#include <manala/tools.h>

StreamPolicy stringToStreamPolicy(std::string name)
{
    if(name.compare(std::string("none")) == 0)
        return DECAF_STREAM_NONE;
    else if(name.compare(std::string("single")) == 0)
        return DECAF_STREAM_SINGLE;
    else if(name.compare(std::string("double")) == 0)
        return DECAF_STREAM_DOUBLE;
    else
    {
        std::cerr<<"WARNING: unknown stream policy name: "<<name<<"."<<std::endl;
        return DECAF_STREAM_NONE;
    }
}

StorageType stringToStoragePolicy(std::string name)
{
    if(name.compare(std::string("none")) == 0)
        return DECAF_STORAGE_NONE;
    else if(name.compare(std::string("mainmem")) == 0)
        return DECAF_STORAGE_MAINMEM;
    else if(name.compare(std::string("file")) == 0)
        return DECAF_STORAGE_FILE;
    else if(name.compare(std::string("dataspace")) == 0)
        return DECAF_STORAGE_DATASPACE;
    else
    {
        std::cerr<<"WARNING: unknown storage type: "<<name<<"."<<std::endl;
        return DECAF_STORAGE_NONE;
    }
}

StorageCollectionPolicy stringToStorageCollectionPolicy(std::string name)
{
    if(name.compare(std::string("greedy")) == 0)
        return DECAF_STORAGE_COLLECTION_GREEDY;
    else if(name.compare(std::string("lru")) == 0)
        return DECAF_STORAGE_COLLECTION_LRU;
    else
    {
        std::cerr<<"WARNING: unknown storage collection policy: "<<name<<"."<<std::endl;
        return DECAF_STORAGE_COLLECTION_GREEDY;
    }

}

FramePolicyManagment stringToFramePolicyManagment(std::string name)
{
    if(name.compare(std::string("none")) == 0)
        return DECAF_FRAME_POLICY_NONE;
    else if(name.compare(std::string("seq")) == 0)
        return DECAF_FRAME_POLICY_SEQ;
    else if(name.compare(std::string("recent")) == 0)
        return DECAF_FRAME_POLICY_RECENT;
    else if(name.compare(std::string("lowhigh")) == 0)
            return DECAF_FRAME_POLICY_LOWHIGH;
    else
    {
        std::cerr<<"WARNING: unknown frame policy type: "<<name<<"."<<std::endl;
        return DECAF_FRAME_POLICY_NONE;
    }
}
