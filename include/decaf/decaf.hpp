//---------------------------------------------------------------------------
//
// decaf top-level interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_HPP
#define DECAF_HPP

#include <bredala/data_model/pconstructtype.h>

#include <dlfcn.h>
#include <map>
#include <string>
#include <vector>
#include <decaf/dataflow.hpp>

// transport layer specific types
#ifdef TRANSPORT_MPI
#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/comm.h>
#endif

#include <decaf/types.hpp>
#include <decaf/workflow.hpp>

namespace decaf
{
// node or link in routing table
struct RoutingNode
{
    int idx;                             ///<  node index in workflow
    set<int> wflow_in_links;             ///<  input links in workflow for this node
    set<int> ready_in_links;             ///<  input links with ready data items
    bool done;                           ///<  node is all done (will be set to true when receiving the quit message)
};
struct RoutingLink
{
    int idx;                             ///<  link index in workflow
    int nin;                             ///<  number of ready input data items
    bool done;                           ///<  link is all done (will be set to true when receiving the quit message)
};

class Decaf
{
public:
    Decaf(CommHandle world_comm,
          Workflow& workflow);
    ~Decaf();
    void err();
    void run_links(bool run_once);       // run the link dataflows
    void print_workflow();               // debug: print the workflow

    // returns true = ok, false = function terminate() has been called
    //! put a message on all outbound links
    bool put(pConstructData container);

    //! put a message on a particular outbound link
    // returns true = ok, false = function terminate() has been called
    bool put(pConstructData container, int i);

    //! put a message on outbound link(s) associated to the output port
    // returns true = ok, false = function terminate() has been called
    bool put(pConstructData container, string port);

    //! get a message from an inbound link associated to the input port
    // returns true = process message, false = error occured or quit message
    bool get(pConstructData container, string port);

    //! get message from all input ports of a node
    // returns true = process messages, false = error occured or all quit messages
    // if an input port is closed, the container in the returned map is empty
    bool get(map<string, pConstructData> &containers);

    //! get message from all input links of a node
    // returns true = process messages, false = error occured or all quit messages
    // if an input link is closed, the container in the returned map is empty
    bool get(map<int, pConstructData> &containers);

    //! get messages from all inbound links
    // returns true = process messages, false = break (quit received)
    bool get(vector< pConstructData >& containers);

    //! checks if there are still alive input Dataflows for this node
    bool allQuit();

    //! terminates a node task by sending a quit message to the rest of the workflow
    void terminate();

    //! whether my rank belongs to this workflow node, identified by the name of its func field
    bool my_node(const char* name);

    //! return a pointer to a dataflow, identified by its index in the workflow structure
    Dataflow* dataflow(int i);

    //! returns the total number of dataflows build by this instance of Decaf
    unsigned int nb_dataflows();

    //! clears the buffers of the output dataflows
    void clearBuffers(TaskType role);

    //! returns a handle for this node's producer communicator
    CommHandle prod_comm_handle();
    //! returns a handle for this node's consumer communicator
    CommHandle con_comm_handle();
    //! returns the size of the producers
    int prod_comm_size();
    //! returns the size of the consumers
    int con_comm_size();

    int local_comm_size();              // Return the size of the communicator of the local task
    CommHandle local_comm_handle();     // Return the communicator of the local task
    int local_comm_rank();              // Return the rank of the process within the local rank
    int prod_comm_size(int i);          ///< return the size of the communicator of the producer of the in dataflow i
    int con_comm_size(int i);           ///< return the size of the communicator of the consumer of the out dataflow i

    //! returns the size of the workflow
    int workflow_comm_size();
    //! returns the rank within the workflow
    int workflow_comm_rank();

    //! return a pointer to this node's producer communicator
    Comm* prod_comm();
    //! return a pointer to this node's consumer communicator
    Comm* con_comm();

    Comm* world;

private:
    // builds a vector of dataflows for all links in the workflow
    void build_dataflows(vector<Dataflow*>& dataflows);

    // loads a dataflow module (if it has not been loaded) containing a callback function name
    // returns a pointer to the function
    void* load_dflow(
            map<string, void*>& mods,
            string mod_name,
            string func_name);

    // unloads all loaded modules
    void
    unload_mods(map<string, void*>& mods);

    // TODO: make the following 5 functions members of ConstructData?

    // returns the source type (node, link) of a message
    TaskType
    src_type(pConstructData in_data);

    // returns the workflow link id of a message
    // id is the index in the workflow data structure of the link
    int
    link_id(pConstructData in_data);

    // returns the workflow destination id of a message
    // destination could be a node or link depending on the source type
    // id is the index in the workflow data structure of the destination entity
    int
    dest_id(pConstructData in_data);

    // routes input data to a callback function
    // returns 0 on success, -1 if all my nodes and links are done; ie, shutdown
    int
    router(list< pConstructData >& in_data,
           vector<int>& ready_ids,
           vector<int>& ready_types);

    // Check for producers/consumers with all their inputs
    // check for dataflows with inputs (each dataflow has only one)
    // and fill ready_ids and ready_types with the ready nodes and links
    void
    ready_nodes_links(vector<int>& ready_ids,
                      vector<int>& ready_types);

    // check if all done
    // 0: still active entries
    // -1: all done
    int all_done();

    // return index in my_nodes_ of workflow node id
    // -1: not found
    int my_node(int workflow_id);

    // return index in my_links_ of workflow link id
    // -1: not found
    int my_link(int workflow_id);

    // data members
    CommHandle world_comm_;                     // handle to original world communicator
    int workflow_size_;                         // Size of the workflow
    int workflow_rank_;                         // Rank within the workflow
    Workflow workflow_;                         // workflow
    int err_;                                   // last error
    vector<RoutingNode> my_nodes_;              // indices of my workflow nodes
    vector<RoutingLink> my_links_;              // indices of my workflow links
    vector<Dataflow*>   dataflows;              // all dataflows for the entire workflow
    vector<Dataflow*>   out_dataflows;          // all my outbound dataflows
    vector<Dataflow*>   link_in_dataflows;      // all my inbound dataflows in case I am a link
    vector<pair<Dataflow*, int>> node_in_dataflows; // all my inbound dataflows in case I am a node, and the corresponding index in the vector dataflows

    map<string, Dataflow*> inPortMap;		    // Map between an input port and its associated Dataflow
    map<string, vector<Dataflow*>> outPortMap;  // Map between an output port and the list of its associated Dataflow

    int tokens_;                                // Number of empty messages to generate before doing a real get

};

} // namespace

// every user application needs to implement the following run function
//void run(Workflow& workflow); //TODO not used anymore?

#endif
