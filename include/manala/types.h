#ifndef MANALA_TYPES_H
#define MANALA_TYPES_H

using namespace std;

enum StreamPolicy
{
    DECAF_STREAM_NONE,
    DECAF_STREAM_SINGLE,
    DECAF_STREAM_DOUBLE,
};

enum StorageType
{
    DECAF_STORAGE_NONE,
    DECAF_STORAGE_MAINMEM,
    DECAF_STORAGE_FILE,
    DECAF_STORAGE_DATASPACE,
};

enum FrameCommand
{
    DECAF_FRAME_COMMAND_REMOVE,
    DECAF_FRAME_COMMAND_REMOVE_UNTIL,
    DECAF_FRAME_COMMAND_REMOVE_UNTIL_EXCLUDED,
};

enum StorageCollectionPolicy
{
    DECAF_STORAGE_COLLECTION_GREEDY,
    DECAF_STORAGE_COLLECTION_LRU
};

enum FramePolicyManagment
{
    DECAF_FRAME_POLICY_NONE,
    DECAF_FRAME_POLICY_SEQ,
    DECAF_FRAME_POLICY_RECENT,
    DECAF_FRAME_POLICY_LOWHIGH
};


struct ManalaInfo
{
    string stream;              // Type of stream policy to use (none, single, double)
    string frame_policy;        // Policy to use to manage the incoming frames
    unsigned int prod_freq_output;              // Output frequency of the producer
    string storage_policy;                      // Type of storage collection to use
    vector<StorageType> storages;               // Different level of storage availables
    vector<unsigned int> storage_max_buffer;    // Maximum number of frame
    unsigned int low_frequency;                 // Garantee forward frequency. Used for lowhigh framemanager
    unsigned int high_frequency;                // High forward frequency. May not be achieved. Used for lowhigh framemanager
};

#endif
