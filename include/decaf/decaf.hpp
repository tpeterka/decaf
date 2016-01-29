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

#include <decaf/data_model/constructtype.h>

#include <dlfcn.h>
#include <map>
#include "dataflow.hpp"

// transport layer specific types
#ifdef TRANSPORT_MPI
#include "transport/mpi/types.h"
#include "transport/mpi/comm.hpp"
#include "transport/mpi/data.hpp"
#endif

#include "types.hpp"
#include "data.hpp"

namespace decaf
{
    // node or link in routing table
    struct RoutingNode
    {
        int idx;                             // node index in workflow
        vector<int> in_links;                // links with ready input data items
        bool done;                           // node is all done
    };
    struct RoutingLink
    {
        int idx;                             // link index in workflow
        int nin;                             // number of ready input data items
        bool done;                           // link is all done
    };

    class Decaf
    {
    public:
        Decaf(CommHandle world_comm,
              Workflow& workflow);
        ~Decaf()                      { delete world;                 }
        void err()                    { ::all_err(err_);              }
        void run(void (*pipeliner)(Dataflow*),
                 void (*checker)(Dataflow*),
                 vector<int>& sources);
        Comm* world;

    private:
        // builds a vector of dataflows for all links in the workflow
        void build_dataflows(vector<Dataflow*>& dataflows,
                             void (*pipeliner)(Dataflow*),
                             void (*checker)(Dataflow*))
            {
                DecafSizes decaf_sizes;
                for (size_t i = 0; i < workflow_.links.size(); i++)
                {
                    int prod  = workflow_.links[i].prod;    // index into workflow nodes
                    int dflow = i;                          // index into workflow links
                    int con   = workflow_.links[i].con;     // index into workflow nodes
                    decaf_sizes.prod_size           = workflow_.nodes[prod].nprocs;
                    decaf_sizes.dflow_size          = workflow_.links[dflow].nprocs;
                    decaf_sizes.con_size            = workflow_.nodes[con].nprocs;
                    decaf_sizes.prod_start          = workflow_.nodes[prod].start_proc;
                    decaf_sizes.dflow_start         = workflow_.links[dflow].start_proc;
                    decaf_sizes.con_start           = workflow_.nodes[con].start_proc;
                    Decomposition prod_dflow_redist =
                        stringToDecomposition(workflow_.links[dflow].prod_dflow_redist);
                    Decomposition dflow_con_redist =
                        stringToDecomposition(workflow_.links[dflow].dflow_con_redist);
                    dataflows.push_back(new Dataflow(world_comm_,
                                                     decaf_sizes,
                                                     pipeliner,
                                                     checker,
                                                     prod,
                                                     dflow,
                                                     con,
                                                     prod_dflow_redist,
                                                     dflow_con_redist));
                    dataflows[i]->err();
                }
            }

        // loads a node module (if it has not been loaded) containing a callback function name
        // returns a pointer to the function
        void* load_node(
            map<string, void*>& mods,      // (name, handle) of modules dynamically loaded so far
            string mod_name,               // module name (path) to be loaded
            string func_name)              // function name to be called
            {
                map<string, void*>::iterator it;              // iterator into the modules
                pair<string, void*> p;                        // (module name, module handle)
                pair<map<string, void*>::iterator, bool> ret; // return value of insertion into mods
                void(*func)(void*,                            // ptr to callback func
                            vector<Dataflow*>*,
                            vector< shared_ptr<ConstructData> >*);

                if ((it = mods.find(mod_name)) == mods.end())
                {
                    void* f = dlopen(mod_name.c_str(), RTLD_LAZY);
                    if (!f)
                        fprintf(stderr, "Error: module %s could not be loaded\n",
                                mod_name.c_str());
                    p = make_pair(mod_name, f);
                    ret = mods.insert(p);
                    it = ret.first;
                }
                return dlsym(it->second, func_name.c_str());
            }

        // loads a dataflow module (if it has not been loaded) containing a callback function name
        // returns a pointer to the function
        void* load_dflow(
            map<string, void*>& mods,      // (name, handle) of modules dynamically loaded so far
            string mod_name,               // module name (path) to be loaded
            string func_name)              // function name to be called
            {
                map<string, void*>::iterator it;              // iterator into the modules
                pair<string, void*> p;                        // (module name, module handle)
                pair<map<string, void*>::iterator, bool> ret; // return value of insertion into mods
                void(*func)(void*,                            // ptr to callback func
                            Dataflow*,
                            shared_ptr<ConstructData>);

                if ((it = mods.find(mod_name)) == mods.end())
                {
                    void* f = dlopen(mod_name.c_str(), RTLD_LAZY);
                    if (!f)
                        fprintf(stderr, "Error: module %s could not be loaded\n",
                                mod_name.c_str());
                    p = make_pair(mod_name, f);
                    ret = mods.insert(p);
                    it = ret.first;
                }
                return dlsym(it->second, func_name.c_str());
            }

        // unloads all loaded modules
        void
        unload_mods(map<string, void*>& mods) // (name, handle) of modules dynamically loaded so far
            {
                map<string, void*>::iterator it;              // iterator into the modules
                for (it = mods.begin(); it != mods.end(); it++)
                    dlclose(it->second);
            }

        // TODO: make the following 5 functions members of ConstructData?

        // returns the source type (prod, dflow, con) of a message
        TaskType
        src_type(shared_ptr<ConstructData> in_data)   // input message
            {
                shared_ptr<BaseConstructData> ptr = in_data->getData(string("src_type"));
                if (ptr)
                {
                    shared_ptr<SimpleConstructData<TaskType> > type =
                        dynamic_pointer_cast<SimpleConstructData<TaskType> >(ptr);
                    return type->getData();
                }
                return DECAF_NONE;
            }

        // returns the workflow link id of a message
        // id is the index in the workflow data structure of the link
        int
        link_id(shared_ptr<ConstructData> in_data)   // input message
            {
                shared_ptr<BaseConstructData> ptr = in_data->getData(string("link_id"));
                if (ptr)
                {
                    shared_ptr<SimpleConstructData<int> > link =
                        dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                    return link->getData();
                }
                return -1;
            }

        // returns the workflow destination id of a message
        // destination could be a node or link depending on the source type
        // id is the index in the workflow data structure of the destination entity
        int
        dest_id(shared_ptr<ConstructData> in_data)   // input message
            {
                shared_ptr<BaseConstructData> ptr = in_data->getData(string("dest_id"));
                if (ptr)
                {
                    shared_ptr<SimpleConstructData<int> > dest =
                        dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                    return dest->getData();
                }
                return -1;
            }

        // tests whether a message is a quit command
        bool
        test_quit(shared_ptr<ConstructData> in_data)   // input message
            {
                return in_data->hasData(string("decaf_quit"));
            }

        // sets a quit message into a container
        // caller still needs to send the message
        void
        set_quit(shared_ptr<ConstructData> out_data)   // output message
            {
                shared_ptr<SimpleConstructData<int> > data  =
                    make_shared<SimpleConstructData<int> >(1);
                out_data->appendData(string("decaf_quit"), data,
                                      DECAF_NOFLAG, DECAF_PRIVATE,
                                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
            }

        // routes input data to a callback function
        // returns 0 on success, -1 if all my nodes and links are done; ie, shutdown
        int
        router(list< shared_ptr<ConstructData> >& in_data,        // input messages
               vector<int>& node_link_ids,                   // (output) workflow node or link ids
               vector<int>& types)                           // (output) callback types
                                                             // DECAF_PROD, DECAF_DFLOW, DECAF_CON,
                                                             // DECAF_BOTH; DECAF_NONE if no match
            {
                for (list< shared_ptr<ConstructData> >::iterator it = in_data.begin();
                     it != in_data.end(); it++)
                {
                    // is destination a workflow node or a link
                    int dest_node = -1;
                    int dest_link = -1;

                    if (src_type(*it) & DECAF_PROD || src_type(*it) & DECAF_CON)
                        dest_link = dest_id(*it);
                    if (src_type(*it) & DECAF_DFLOW)
                        dest_node = dest_id(*it);

                    if (dest_node >= 0)            // destination is a node
                    {
                        if (test_quit(*it))
                            my_nodes_[dest_node].done = true;
                        else
                        {
                            for (size_t j = 0; j < my_nodes_.size(); j++)
                            {
                                if (dest_id(*it) == my_nodes_[j].idx)
                                    my_nodes_[j].in_links.push_back(link_id(*it));
                            }
                        }                          // else (ie, not done)
                    }                              // node

                    else                           // destination is a link
                    {
                        if (test_quit(*it))
                            my_links_[dest_link].done = true;

                        else
                        {
                            // count number of input items for each dataflow
                            for (size_t j = 0; j < my_links_.size(); j++)
                            {
                                if (dest_id(*it) == my_links_[j].idx)
                                    my_links_[j].nin++;
                            }
                        }                          // else (ie, not done)
                    }                              // link
                }                                  // in_data iterator


                // debug
                // if (world->rank() == 7)
                //     fprintf(stderr, "r1.5: in_links size %ld in_links[0] %d\n",
                //             my_nodes_[0].in_links.size(), my_nodes_[0].in_links[0]);

                // TODO: got here, but in_links keeps growing

                // check for ready nodes and links having all their inputs
                ready_nodes_links(node_link_ids, types);

                return(all_done());
            }                                      // router

        // Check for producers/consumers with all their inputs
        // check for dataflows with inputs (each dataflow has only one)
        // and fill node_link_ids and types with the ready nodes and links
        void
        ready_nodes_links(vector<int>& node_link_ids,        // (output) workflow node or link ids
                          vector<int>& types)                // (output) callback types
                                                             // DECAF_BOTH or DECAF_DFLOW
            {
                for (size_t i = 0; i < my_nodes_.size(); i++) // my nodes
                {
                    if (!my_nodes_[i].done && my_nodes_[i].in_links.size())
                    {
                        // sort my_node and workflow in_links for this node
                        set<int> my_links_(my_nodes_[i].in_links.begin(),
                                          my_nodes_[i].in_links.end());
                        set<int> wf_links(workflow_.nodes[my_nodes_[i].idx].in_links.begin(),
                                          workflow_.nodes[my_nodes_[i].idx].in_links.end());

                        // debug
                        // if (world->rank() == 7)
                        //     fprintf(stderr, "my_links size %ld my_links[0] %d "
                        //             "wf_links size %ld wf_links[0] %d\n",
                        //             my_links_.size(), *(my_links_.begin()),
                        //             wf_links.size(), *(wf_links.begin()));

                        // my_links_ is a subset of wf_links
                        if (includes(wf_links.begin(), wf_links.end(),
                                     my_links_.begin(), my_links_.end()))
                        {
                            // debug
                            // fprintf(stderr, "adding node %d my_links size %ld "
                            //         "wf_links size %ld\n",
                            //         my_nodes_[i].idx, my_links_.size(), wf_links.size());

                            node_link_ids.push_back(my_nodes_[i].idx);
                            types.push_back(DECAF_PROD | DECAF_CON);
                        }
                    }
                }                                             // my nodes
                for (size_t i = 0; i < my_links_.size(); i++) // my links
                {
                    if (!my_links_[i].done && my_links_[i].nin >= 1)
                    {
                        node_link_ids.push_back(my_links_[i].idx);
                        types.push_back(DECAF_DFLOW);
                        my_links_[i].nin--;
                    }
                }                                             // my links
            }

        // check if all done
        // 0: still active entries
        // -1: all done
        int all_done()
            {
                for (size_t i = 0; i < my_nodes_.size(); i++)
                {
                    if (!my_nodes_[i].done)
                        return 0;
                }
                for (size_t i = 0; i < my_links_.size(); i++)
                {
                    if (!my_links_[i].done)
                        return 0;
                }
                return -1;
            }

        // data members
        CommHandle world_comm_;                    // handle to original world communicator
        Workflow workflow_;                        // workflow
        int err_;                                  // last error
        vector<RoutingNode> my_nodes_;             // indices of workflow nodes in my process
        vector<RoutingLink> my_links_;             // indices of workflow links in my process
    };

} // namespace

// constructor
decaf::
Decaf::Decaf(CommHandle world_comm,
             Workflow& workflow) :
    world_comm_(world_comm),
    workflow_(workflow)
{
    world = new Comm(world_comm);

    // routing table is simply vectors of workflow nodes and links that belong to my process

    // add my nodes
    for (size_t i = 0; i < workflow_.nodes.size(); i++)
    {
        if (workflow_.my_node(world->rank(), i))
        {
            // TODO: C++11 initialization of POD?
            RoutingNode nl;
            nl.idx  = i;
            nl.done = false;
            my_nodes_.push_back(nl);
        }
    }

    // add my links
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(world->rank(), i))
        {
            // TODO: C++11 initialization of POD?
            RoutingLink nl;
            nl.idx  = i;
            nl.nin  = 0;
            nl.done = false;
            my_links_.push_back(nl);
        }
    }
}

// runs the workflow
void
decaf::
Decaf::run(void (*pipeliner)(decaf::Dataflow*),    // custom pipeliner code
           void (*checker)(decaf::Dataflow*),      // custom resilience code
           vector<int>& sources)                   // workflow source nodes
{
    // incoming messages
    list< shared_ptr<ConstructData> > containers;

    // collect dataflows
    vector<decaf::Dataflow*> dataflows;            // dataflows for all links in the workflow
    build_dataflows(dataflows, pipeliner, checker);
    vector<decaf::Dataflow*> out_dataflows;        // outbound dataflows for one node

    // inbound dataflows for this process
    vector<Dataflow*> link_in_dataflows;           // dataflows for my links
    vector<Dataflow*> node_in_dataflows;           // dataflows that ar inputs to my nodes

    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        // the input dataflows on which I should listen for messages
        if (workflow_.my_link(world->rank(), i))        // I am a link and this dataflow is me
            link_in_dataflows.push_back(dataflows[i]);
        if (workflow_.my_in_link(world->rank(), i))     // I am a node and this dataflow is an input
            node_in_dataflows.push_back(dataflows[i]);
    }

    // dynamically loaded modules (plugins)
    void(*node_func)(void*,                       // ptr to producer or consumer callback func
                     vector<Dataflow*>*,
                     vector< shared_ptr<ConstructData> >*);
    void(*dflow_func)(void*,                      // ptr to dataflow callback func in a module
                      Dataflow*,
                      shared_ptr<ConstructData>);
    map<string, void*> mods;                      // modules dynamically loaded so far

    // sources, ie, tasks that don't need to wait for input in order to start
    for (size_t s = 0; s < sources.size(); s++)
    {
        int i = sources[s];

        // check if this process belongs to the node
        if (!workflow_.my_node(world->rank(), i))
            continue;

        // fill out_dataflows for this node
        for (size_t j = 0; j < workflow_.nodes[i].out_links.size(); j++)
            out_dataflows.push_back(dataflows[workflow_.nodes[i].out_links[j]]);

        node_func = (void(*)(void*,
                             vector<Dataflow*>*,
                             vector< shared_ptr<ConstructData> >*))
            load_node(mods, workflow_.nodes[i].path, workflow_.nodes[i].func);

        node_func(workflow_.nodes[i].args,
                  &out_dataflows,
                  NULL);
    }

    // remaining (nonsource) tasks and dataflows are driven by receiving messages
    while (1)
    {
        // get incoming data
        for (size_t i = 0; i < link_in_dataflows.size(); i++) // I am a link
        {
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            if (link_in_dataflows[i]->get(container, DECAF_DFLOW))
            {
                // fprintf(stderr, "2: dataflow got all messages\n");
                containers.push_back(container);
            }
        }
        for (size_t i = 0; i < node_in_dataflows.size(); i++) // I am a node
        {
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            if (node_in_dataflows[i]->get(container, DECAF_CON))
            {
                // TODO: only getting messages at rank 0
                if (world->rank() < 4)
                    fprintf(stderr, "3: node got all messages\n");
                containers.push_back(container);
            }
        }

        // route the message: decide what dataflows and tasks should accept it
        vector<int> nls;                                // index of node or link in workflow
        vector<int> types;                              // type of node or link
        if (containers.size() && router(containers, nls, types) == -1)
            break;

        // check for termination and propagate the quit message
        for (list< shared_ptr<ConstructData> >::iterator it = containers.begin();
             it != containers.end(); it++)
        {
            // add quit flag to containers object, initialize to false
            if (test_quit(*it))
            {
                // send quit to destinations
                shared_ptr<ConstructData> container = make_shared<ConstructData>();
                set_quit(container);
                for (size_t i = 0; i < nls.size(); i++)
                {
                    for (size_t j = 0; j < workflow_.nodes[nls[i]].out_links.size(); j++)
                        dataflows[workflow_.nodes[nls[i]].out_links[j]]->
                            put(container, DECAF_PROD | DECAF_CON);
                }
            }
        }

        for (size_t i = 0; i < nls.size(); i++)
        {
            // a workflow node (producer, consumer, or both)
            if (types[i] & (DECAF_PROD | DECAF_CON))
            {
                // fill out_dataflows for this node
                out_dataflows.clear();
                for (size_t j = 0; j < workflow_.nodes[nls[i]].out_links.size(); j++)
                {
                    // fprintf(stderr, "4: adding out_dataflow id %d\n",
                    //         workflow_.nodes[nls[i]].out_links[j]);
                    out_dataflows.push_back(dataflows[workflow_.nodes[nls[i]].out_links[j]]);
                }

                // fill node_containers for this node
                vector< shared_ptr<ConstructData> > node_containers; // input messages for 1 node
                for (size_t j = 0; j < workflow_.nodes[nls[i]].in_links.size(); j++)
                {
                    for (list< shared_ptr<ConstructData> >::iterator it = containers.begin();
                         it != containers.end(); it++)
                    {
                        if (link_id(*it) == workflow_.nodes[nls[i]].in_links[j])
                            node_containers.push_back(*it);
                    }
                }

                node_func = (void(*)(void*,             // load the function
                                     vector<Dataflow*>*,
                                     vector< shared_ptr<ConstructData> >*))
                    load_node(mods,
                              workflow_.nodes[nls[i]].path,
                              workflow_.nodes[nls[i]].func);

                node_func(workflow_.nodes[nls[i]].args, // call it
                          &out_dataflows,
                          &node_containers);

                // remove used messages only after the callback has been called
                for (size_t j = 0; j < workflow_.nodes[nls[i]].in_links.size(); j++)
                {
                    list< shared_ptr<ConstructData> >::iterator it = containers.begin();
                    // using while instead of for: erase inside loop disrupts the iteration
                    while (it != containers.end())
                    {
                        if (link_id(*it) == workflow_.nodes[nls[i]].in_links[j])
                            // erase kills the std iterator but returns the next one
                            it = containers.erase(it);
                        else
                            it++;
                    }
                }
            }

            // a workflow link (dataflow)
            else if (types[i] & DECAF_DFLOW)
            {
                list< shared_ptr<ConstructData> >::iterator it = containers.begin();
                // using while instead of for: erase inside loop disrupts the iteration
                while (it != containers.end())
                {
                    if (link_id(*it) == nls[i])
                    {
                        dflow_func = (void(*)(void*,            // load the function
                                              Dataflow*,
                                              shared_ptr<ConstructData>))
                            load_dflow(mods,
                                       workflow_.links[nls[i]].path,
                                       workflow_.links[nls[i]].func);

                        dflow_func(workflow_.links[nls[i]].args, // call it
                                   dataflows[nls[i]],
                                   *it);
                        // erase kills the std iterator but returns the next one
                        it = containers.erase(it);
                    }
                    else
                        it++;
                }
            }                                           // workflow link
        }                                               // for i = 0; i < nls.size()
        usleep(10);
    }                                                       // while (1)

    // cleanup
    unload_mods(mods);
    for (size_t i = 0; i < dataflows.size(); i++)
        delete dataflows[i];
}

#endif
