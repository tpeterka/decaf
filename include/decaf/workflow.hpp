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
	/*WorkflowLink(int prod_,
                 int con_,
                 int start_proc_,
                 int nprocs_,
                 string func_,
                 string path_,
                 string prod_dflow_redist_,
	             string dflow_con_redist_,
	             vector<ContractKey> list_keys_,
	             Check_level check_level_) :
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
		check_level(check_level_)				{}*/
    int prod;                   // index in vector of all workflow nodes of producer
    int con;                    // index in vector of all workflow nodes of consumer
    int start_proc;             // starting process rank in world communicator for the dataflow
    int nprocs;                 // number of processes in the dataflow
    string func;                // name of dataflow callback
    void* args;                 // callback arguments
    string path;                // path to callback function module
    string prod_dflow_redist;   // redistribution component between producer and dflow
    string dflow_con_redist;    // redistribution component between dflow and consumer

	string srcPort;				// Portname of the source
	string destPort;			// Portname of the dest

	// The following two are only relevant if the dataflow is related to a contract
	vector<ContractKey> list_keys;   // pairs key/type of the data to be exchanged b/w the producer and consumer
	Check_level check_level;						  // level of checking for the types of data to be exchanged

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

	string json_filename = json_path;
    if(json_filename.length() == 0)
    {
        fprintf(stderr, "No name filename provided for the JSON file. Falling back on the DECAF_JSON environment variable\n");
        const char* env_path = std::getenv("DECAF_JSON");
        if(env_path == NULL)
        {
            fprintf(stderr, "ERROR: The environment variable DECAF_JSON is not defined. Unable to find the workflow graph definition.\n");
            exit(1);
        }
		json_filename = string(env_path);
    }

    try {

      bpt::ptree root;
      
      /*
       * Use Boost::property_tree to read/parse the JSON file. This 
       * creates a property_tree object which we'll then query to get 
       * the information we want.
       *
       * N.B. Unlike what is provided by most JSON/XML parsers, the 
       * property_tree is NOT a DOM tree, although processing it is somewhat
       * similar to what you'd do with a DOM tree. Refer to the Boost documentation
       * for more information.
       */
      
      bpt::read_json( json_filename, root );

      /* 
       * iterate over the list of nodes, creating and populating WorkflowNodes as we go
       */
      for( auto &&v : root.get_child( "workflow.nodes" ) ) {
		WorkflowNode node;
		node.out_links.clear();
		node.in_links.clear();
		/* we defer actually linking nodes until we read the edge list */

		node.start_proc = v.second.get<int>("start_proc");
		node.nprocs = v.second.get<int>("nprocs");
		node.func = v.second.get<string>("func");

		workflow.nodes.push_back( node );
      }

	  string sCheck = root.get<string>("workflow.filter_level");
	  Check_level check_level = stringToCheckLevel(sCheck);

      /* 
       * similarly for the edges
       */
      for( auto &&v : root.get_child( "workflow.edges" ) ) {
		WorkflowLink link;

		/* link the nodes correctly(?) */
		link.prod = v.second.get<int>("source");
		link.con = v.second.get<int>("target");

        workflow.nodes.at( link.prod ).out_links.push_back( workflow.links.size() );
        workflow.nodes.at( link.con ).in_links.push_back( workflow.links.size() );
	 
		link.start_proc = v.second.get<int>("start_proc");
		link.nprocs = v.second.get<int>("nprocs");
		link.prod_dflow_redist = v.second.get<string>("prod_dflow_redist");

		if(link.nprocs != 0){
			link.path = v.second.get<string>("path");
			link.func = v.second.get<string>("func");
			link.dflow_con_redist = v.second.get<string>("dflow_con_redist");
		}

		boost::optional<string> srcP = v.second.get_optional<string>("sourcePort");
		boost::optional<string> destP = v.second.get_optional<string>("targetPort");
		if(srcP && destP){
			link.srcPort = srcP.get();
			link.destPort = destP.get();
		}

		boost::optional<bpt::ptree&> pt_keys = v.second.get_child_optional("keys");
		if(pt_keys){
			for(bpt::ptree::value_type &value: pt_keys.get()){
				ContractKey field;
				field.name = value.first;

				// Didn't find a nicer way of doing this...
				auto i = value.second.begin();
				field.type = i->second.get<string>("");
				i++;
				field.period = i->second.get<int>("");
				//////

				link.list_keys.push_back(field);
			}
			link.check_level = check_level;
		}

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
