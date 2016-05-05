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

#include "decaf.hpp"

#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <iostream>

using namespace std;

struct WorkflowNode                          // a producer or consumer
{
    WorkflowNode()                                {}
    WorkflowNode(int start_proc_,
                 int nprocs_,
                 string func_) :
        start_proc(start_proc_),
        nprocs(nprocs_),
        func(func_),
        args(NULL)                                {}
    vector<int> out_links; // indices of outgoing links
    vector<int> in_links;  // indices of incoming links
    int start_proc;        // starting proc rank in world communicator for this producer or consumer
    int nprocs;            // number of processes for this producer or consumer
    string func;           // name of node callback
    void* args;            // callback arguments
    void add_out_link(int link) { out_links.push_back(link); }
    void add_in_link(int link) { in_links.push_back(link); }
};

struct WorkflowLink                          // a dataflow
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

struct Workflow                              // an entire workflow
{
    Workflow()                                    {}
    Workflow(vector<WorkflowNode>& nodes_,
             vector<WorkflowLink>& links_) :
        nodes(nodes_),
        links(links_)                             {}
    vector<WorkflowNode> nodes;              // all the workflow nodes
    vector<WorkflowLink> links;              // all the workflow links
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
    bool my_out_link(int proc, int link)      // whether my process puts output data to this link
        {
            for (size_t i = 0; i< nodes.size(); i++)
            {
                if (proc >= nodes[i].start_proc && // proc is mine
                    proc <  nodes[i].start_proc + nodes[i].nprocs)
                {
                    for (size_t j = 0; j < nodes[i].out_links.size(); j++)
                    {
                        if (nodes[i].out_links[j] == link)
                            return true;
                    }
                }
            }
            return false;
        }
};

#endif
