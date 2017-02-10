//---------------------------------------------------------------------------
//
// decaf typedefs, enums
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_TYPES_HPP
#define DECAF_TYPES_HPP

#include "decaf.hpp"

#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <iostream>

using namespace std;

enum Decomposition
{
    DECAF_ROUND_ROBIN_DECOMP,
    DECAF_CONTIG_DECOMP,
    DECAF_ZCURVE_DECOMP,
    DECAF_BLOCK_DECOMP,
    DECAF_PROC_DECOMP,
    DECAF_NUM_DECOMPS,
};

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

enum FramePolicyManagment
{
    DECAF_FRAME_POLICY_NONE,
    DECAF_FRAME_POLICY_SEQ,
    DECAF_FRAME_POLICY_RECENT,
};

// workflow entity types
typedef unsigned char TaskType;
#define DECAF_NONE      0x00
#define DECAF_NODE      0x01
#define DECAF_LINK      0x02

// communicator types
typedef unsigned char CommTypeDecaf;
#define DECAF_OTHER_COMM      0x00
#define DECAF_PRODUCER_COMM   0x01
#define DECAF_DATAFLOW_COMM   0x02
#define DECAF_CONSUMER_COMM   0x04
#define DECAF_PROD_DFLOW_COMM 0x08
#define DECAF_DFLOW_CON_COMM  0x10

enum DecafError
{
    DECAF_OK,
    DECAF_COMM_SIZES_ERR,
    DECAF_NUM_ERRS,
};

struct DecafSizes
{
    int prod_size;         // size (number of processes) of producer communicator
    int dflow_size;        // size (number of processes) of dataflow communicator
    int con_size;          // size (number of processes) of consumer communicator
    int prod_start;        // starting world process rank of producer communicator
    int dflow_start;       // starting world process rank of dataflow communicator
    int con_start;         // starting world process rank of consumer communicator
    int con_nsteps;        // number of consumer timesteps
};

void
all_err(int err_code)
{
    switch (err_code) {
    case DECAF_OK :
        break;
    case DECAF_COMM_SIZES_ERR :
        fprintf(stderr, "Decaf error: Group sizes of producer, consumer, and dataflow exceed total "
                "size of world communicator\n");
        break;
    default:
        break;
    }
}

Decomposition stringToDecomposition(std::string name)
{
    if(name.compare(std::string("round")) == 0)
        return DECAF_ROUND_ROBIN_DECOMP;
    else if (name.compare(std::string("count")) == 0)
        return DECAF_CONTIG_DECOMP;
    else if (name.compare(std::string("zcurve")) == 0)
        return DECAF_ZCURVE_DECOMP;
    else if (name.compare(std::string("block")) == 0)
        return DECAF_BLOCK_DECOMP;
    else if (name.compare(std::string("proc")) == 0)
        return DECAF_PROC_DECOMP;
    else
    {
        std::cerr<<"ERROR : unknown Decomposition name: "<<name<<". Using count instead."<<std::endl;
        return DECAF_CONTIG_DECOMP;
    }
}

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
        std::cerr<<"ERROR: unknown stream policy name: "<<name<<". Disabling streaming capability."<<std::endl;
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
        std::cerr<<"ERROR: unknown storage type: "<<name<<". Disabling storage capability."<<std::endl;
        return DECAF_STORAGE_NONE;
    }
}

FramePolicyManagment stringToFramePolicyManagment(std::string name)
{
    if(name.compare(std::string("seq")) == 0)
        return DECAF_FRAME_POLICY_SEQ;
    else if(name.compare(std::string("recent")) == 0)
        return DECAF_FRAME_POLICY_RECENT;
    else
    {
        std::cerr<<"ERROR: unknown frame policy type: "<<name<<". Disabling frame managment.\n"<<std::endl;
        return DECAF_FRAME_POLICY_NONE;
    }
}

#endif
