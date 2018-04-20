//---------------------------------------------------------------------------
//
// workflow definition
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_WORKFLOW_HPP
#define DECAF_WORKFLOW_HPP

//#include "decaf.hpp"

#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

#include <manala/types.h>
#include <decaf/tools.hpp>
#include <manala/tools.h>
#include <string>

namespace bpt = boost::property_tree;

using namespace std;

struct WorkflowNode                          /// a producer or consumer
{
    WorkflowNode()                                {}
    WorkflowNode(int start_proc_,
                 int nprocs_,
                 string func_) :
        start_proc(start_proc_),
        nprocs(nprocs_),
        func(func_),
        args(NULL){}
    vector<int> out_links;      ///< indices of outgoing links
    vector<int> in_links;       ///< indices of incoming links
    int start_proc;             ///< starting processor rank (root) in world communicator for this producer or consumer
    int nprocs;                 ///< number of processes for this node (producer or consumer)
    string func;                ///< name of node callback
    void* args;                 ///< callback arguments
    vector<string> inports;     ///< input ports, if available
    vector<string> outports;    ///< output ports, if available
    void add_out_link(int link);
    void add_in_link(int link);
};

struct WorkflowLink                          /// a dataflow
{
    WorkflowLink()                                {}
    /*WorkflowLink(int prod_,			// This constructor is never used
                                 int con_,
                 int start_proc_,
                 int nprocs_,
                 string func_,
                 string path_,
                 string prod_dflow_redist_,
                                 string dflow_con_redist_,
                                 vector<ContractKey> list_keys_,
                                 Check_level check_level_,
                 string stream_) :
        prod(prod_),
        con(con_),
        start_proc(start_proc_),
        nprocs(nprocs_),
        func(func_),
        args(NULL),
        path(path_),
        prod_dflow_redist(prod_dflow_redist_),
                dflow_con_redist(dflow_con_redist_),
                list_keys(list_keys_),
                check_level(check_level_),
        stream(stream_){} */
    int prod;                       // index in vector of all workflow nodes of producer
    int con;                        // index in vector of all workflow nodes of consumer
    int start_proc;                 ///< starting process rank in world communicator for the link
    int nprocs;                     ///< number of processes in the link
    string func;                    ///< name of dataflow callback
    string name;                    ///< name of the link. Should be unique in the workflow
    void* args;                     ///< callback arguments
    string path;                    ///< path to callback function module
    string prod_dflow_redist;       ///< redistribution component between producer and link
    string dflow_con_redist;        ///< redistribution component between link and consumer
    string transport_method;        ///< type of communications (mpi,cci, file)
    ManalaInfo manala_info;         ///< informations relative to the flow control management

    string srcPort;                 ///< portname of the source
    string destPort;                ///< portname of the dest
    int tokens;                     ///< number of empty messages to receive on destPort before a real get (for supporting cycles)

    vector<ContractKey> keys_link;  // List of keys to be exchanged b/w the link and the consumer
    vector<ContractKey> list_keys;  // list of key to be exchanged b/w the producer and consumer or producer and link
    Check_level check_level;        // level of checking for the types of data to be exchanged
    bool bAny;                      // Whether the filtering will check the contracts but keep any other field or not


};

struct Workflow                              /// an entire workflow
{
    Workflow()                                    {}
    Workflow(vector<WorkflowNode>& nodes_,
             vector<WorkflowLink>& links_) :
        nodes(nodes_),
        links(links_)                             {}
    vector<WorkflowNode> nodes;             ///< all the workflow nodes
    vector<WorkflowLink> links;             ///< all the workflow links
    bool my_node(int proc, int node);       ///< whether my process is part of this node

    bool my_link(int proc, int link);       ///< whether my process is part of this link

    bool my_in_link(int proc, int link);    ///< whether my process gets input data from this link

    bool my_out_link(int proc, int link);   ///< whether my process puts output data to this link


    static void
    make_wflow_from_json( Workflow& workflow, const string& json_path );
};

#endif
