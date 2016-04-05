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

// data element for creating typemaps for datatypes
enum DispType
{
    DECAF_OFST,
    DECAF_ADDR,
    DECAF_NUM_DISP_TYPES,
};

struct DataMap {
    CommDatatype base_type; // existing datatype used to create this one
    DispType disp_type;     // diplacement is relative OFST or absolute ADDR
    int count;              // count of each element
    Address disp;           // starting displacement of each element in bytes
    // OFSTs are from the start of the type and ADDRs are from 0x0
};

enum Decomposition
{
    DECAF_ROUND_ROBIN_DECOMP,
    DECAF_CONTIG_DECOMP,
    DECAF_ZCURVE_DECOMP,
    DECAF_NUM_DECOMPS,
};


// task types
typedef unsigned char TaskType;
#define DECAF_NONE      0x00
#define DECAF_PROD      0x01
#define DECAF_DFLOW     0x02
#define DECAF_CON       0x04

// workflow entity types
typedef unsigned char TaskType;
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

struct WorkflowNode        // a producer or consumer
{
    WorkflowNode()                                {}
    WorkflowNode(int start_proc_,
                 int nprocs_,
                 string func_,
                 string path_) :
        start_proc(start_proc_),
        nprocs(nprocs_),
        func(func_),
        args(NULL),
        path(path_)                               {}
    vector<int> out_links; // indices of outgoing links
    vector<int> in_links;  // indices of incoming links
    int start_proc;        // starting proc rank in world communicator for this producer or consumer
    int nprocs;            // number of processes for this producer or consumer
    string func;           // name of node callback
    void* args;            // callback arguments
    string path;           // path to producer and consumer callback function module
    void add_out_link(int link) { out_links.push_back(link); }
    void add_in_link(int link) { in_links.push_back(link); }
};

struct WorkflowLink        // a dataflow
{
    WorkflowLink()                                {}
    WorkflowLink(int prod_,
                 int con_,
                 int start_proc_,
                 int nprocs_,
                 string func_,
                 string path_,
                 string prod_dflow_redist_,
                 string dflow_con_redist_) :
        prod(prod_),
        con(con_),
        start_proc(start_proc_),
        nprocs(nprocs_),
        func(func_),
        args(NULL),
        path(path_),
        prod_dflow_redist(prod_dflow_redist_),
        dflow_con_redist(dflow_con_redist_)       {}
    int prod;                   // index in vector of all workflow nodes of producer
    int con;                    // index in vector of all workflow nodes of consumer
    int start_proc;             // starting process rank in world communicator for the dataflow
    int nprocs;                 // number of processes in the dataflow
    string func;                // name of dataflow callback
    void* args;                 // callback arguments
    string path;                // path to callback function module
    string prod_dflow_redist;   // redistribution component between producer and dflow
    string dflow_con_redist;    // redistribution component between dflow and consumer
};

struct Workflow                 // an entire workflow
{
    Workflow()                                    {}
    Workflow(vector<WorkflowNode>& nodes_,
             vector<WorkflowLink>& links_) :
        nodes(nodes_),
        links(links_)                             {}
    vector<WorkflowNode> nodes; // all the workflow nodes
    vector<WorkflowLink> links; // all the workflow links
    bool my_node(int proc, int node)         // whether my process is part of this node
        {
            return(proc >= nodes[node].start_proc &&
                   proc <  nodes[node].start_proc + nodes[node].nprocs);
        }
    bool my_link(int proc, int link)         // whether my process is part of this link
        {
            return(proc >=links[link].start_proc &&
                   proc < links[link].start_proc + links[link].nprocs);
        }
    bool my_in_link(int proc, int link)      // whether my process gets input data from this link
        {
            for (size_t i = 0; i< nodes.size(); i++)
            {
                if (proc >= nodes[i].start_proc && // proc is mine
                    proc <  nodes[i].start_proc + nodes[i].nprocs)
                {
                    for (size_t j = 0; j < nodes[i].in_links.size(); j++)
                    {
                        if (nodes[i].in_links[j] == link)
                            return true;
                    }
                }
            }
            return false;
        }
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
    else
    {
        std::cerr<<"ERROR : unknown Decomposition name : "<<name<<". Using count instead."<<std::endl;
        return DECAF_CONTIG_DECOMP;
    }
}

#endif
