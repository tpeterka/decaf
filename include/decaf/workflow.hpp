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
#include <fstream>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

namespace bpt = boost::property_tree;

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

  static void
  make_wflow_from_json( Workflow& workflow, const string& json_path )
  {
    string decaf_prefix (getenv("DECAF_PREFIX"));
    if( decaf_prefix.size() == 0 ) {
      fprintf( stderr, "ERROR: environment variable DECAF_PREFIX not defined."
	       "Please export DECAF_PREFIX to point to the root of your decaf"
	       "install directory.\n");
      exit(1);
    }

    std::ifstream fs( decaf_prefix + json_path, std::ifstream::binary );
    if (!fs) {      
      cerr << "Error opening JSON file("
	   << decaf_prefix + json_path
	   << ")"
	   << endl;
      exit(1);
    }

    bpt::ptree root;

    try {
      
      bpt::read_json( decaf_prefix + json_path, root );

      for( auto &&v : root.get_child( "workflow.nodes" ) ) {
	/* 
	 * iterate over the list of nodes, creating and populating WorkflowNodes as we go
	 */
	WorkflowNode node;
	node.out_links.clear();
	node.in_links.clear();
	/* we defer actually linking nodes until we read the edge list */

	node.start_proc = v.second.get<int>("start_proc");
	node.nprocs = v.second.get<int>("nprocs");
	node.func = v.second.get<std::string>("func");
	
	workflow.nodes.push_back( node );
      }

      string path = decaf_prefix + root.get<std::string>("workflow.path");
      cerr << path << endl;
      
      for( auto &&v : root.get_child( "workflow.edges" ) ) {
	
	WorkflowLink link;
	
	/* link the nodes correctly(?) */      
	link.prod = v.second.get<int>("source");
	link.con = v.second.get<int>("target");

	workflow.nodes.at( link.prod ).out_links.push_back( link.con );
	workflow.nodes.at( link.con ).in_links.push_back( link.prod );
	 
	link.path = path;
	link.start_proc = v.second.get<int>("start_proc");
	link.nprocs = v.second.get<int>("nprocs");
	link.func = v.second.get<std::string>("func");
	link.prod_dflow_redist = v.second.get<std::string>("count");
	link.dflow_con_redist = v.second.get<std::string>("count");
	workflow.links.push_back( link );
      }
    }
    catch( const bpt::json_parser_error& jpe ) {
      cerr << "JSON parser exception: " << jpe.what() << endl;
      exit(1);
    }
    catch ( const bpt::ptree_error& pte ) {
      cerr << "property_tree exception: " << pte.what() << endl;
      exit(1);
    }
    
  }
  
};

#endif
