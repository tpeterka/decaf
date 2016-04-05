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
#endif

#include "types.hpp"

namespace decaf
{
    // node or link in routing table
    struct RoutingNode
    {
        int idx;                             // node index in workflow
        set<int> wflow_in_links;             // input links in workflow for this node
        set<int> ready_in_links;             // input links with ready data items
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
                 const vector<int>& sources);
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
                int(*func)(void*,                             // ptr to callback func
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
                int(*func)(void*,                             // ptr to callback func
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

        // returns the source type (node, link) of a message
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

        // routes input data to a callback function
        // returns 0 on success, -1 if all my nodes and links are done; ie, shutdown
        int
        router(list< shared_ptr<ConstructData> >& in_data, // input messages
               vector<int>& ready_ids,                  // (output) ready workflow node or link ids
               vector<int>& ready_types)                // (output) ready node or link types
                                                        // DECAF_NODE | DECAF_LINK
            {
                for (list< shared_ptr<ConstructData> >::iterator it = in_data.begin();
                     it != in_data.end(); it++)
                {
                    // is destination a workflow node or a link
                    int dest_node = -1;
                    int dest_link = -1;

                    if (src_type(*it) & DECAF_NODE)
                        dest_link = dest_id(*it);
                    if (src_type(*it) & DECAF_LINK)
                        dest_node = dest_id(*it);

                    if (dest_node >= 0)               // destination is a node
                    {
                        if (Dataflow::test_quit(*it)) // done
                        {
                            size_t j;
                            for (j = 0; j < my_nodes_.size(); j++)
                            {
                                if (my_nodes_[j].idx == dest_node)
                                    break;
                            }
                            if (!my_nodes_[j].done)
                            {
                                my_nodes_[j].done = true;
                                ready_ids.push_back(dest_node);
                                ready_types.push_back(DECAF_NODE);
                            }
                        }
                        else                          // not done
                        {
                            for (size_t j = 0; j < my_nodes_.size(); j++)
                            {
                                if (dest_id(*it) == my_nodes_[j].idx)
                                    my_nodes_[j].ready_in_links.insert(link_id(*it));
                            }
                        }                             // not done
                    }                                 // node

                    else                              // destination is a link
                    {
                        if (Dataflow::test_quit(*it)) // done
                        {
                            size_t j;
                            for (j = 0; j < my_links_.size(); j++)
                            {
                                if (my_links_[j].idx == dest_link)
                                    break;
                            }
                            if (!my_links_[j].done)
                            {
                                my_links_[j].done = true;
                                ready_ids.push_back(dest_link);
                                ready_types.push_back(DECAF_LINK);
                            }
                        }
                        else                          // not done
                        {
                            // count number of input items for each dataflow
                            for (size_t j = 0; j < my_links_.size(); j++)
                            {
                                if (dest_id(*it) == my_links_[j].idx)
                                    my_links_[j].nin++;
                            }
                        }                             // not done
                    }                                 // link
                }                                     // in_data iterator

                // check for ready nodes and links having all their inputs
                ready_nodes_links(ready_ids, ready_types);

                return(all_done());
            }                                      // router

        // Check for producers/consumers with all their inputs
        // check for dataflows with inputs (each dataflow has only one)
        // and fill ready_ids and ready_types with the ready nodes and links
        void
        ready_nodes_links(vector<int>& ready_ids,      // (output) ready workflow node or link ids
                          vector<int>& ready_types)    // (output) ready workflow types
                                                       // DECAF_NODE | DECAF_LINK
            {
                for (size_t i = 0; i < my_nodes_.size(); i++) // my nodes
                {
                    // some of the in links for this node are ready
                    if (!my_nodes_[i].done && my_nodes_[i].ready_in_links.size())
                    {
                        // are the workflow links a subset of the ready input links
                        // ie, are all the workflow links ready (have data)
                        if (includes(my_nodes_[i].ready_in_links.begin(),
                                     my_nodes_[i].ready_in_links.end(),
                                     my_nodes_[i].wflow_in_links.begin(),
                                     my_nodes_[i].wflow_in_links.end()))
                        {
                            ready_ids.push_back(my_nodes_[i].idx);
                            ready_types.push_back(DECAF_NODE);
                            my_nodes_[i].ready_in_links.clear();
                        }
                    }

                    // this node does not have any workflow in links
                    if (!my_nodes_[i].done && !my_nodes_[i].wflow_in_links.size())
                    {
                        ready_ids.push_back(my_nodes_[i].idx);
                        ready_types.push_back(DECAF_NODE);
                    }
                }                                             // my nodes

                for (size_t i = 0; i < my_links_.size(); i++) // my links
                {
                    if (!my_links_[i].done && my_links_[i].nin >= 1)
                    {
                        ready_ids.push_back(my_links_[i].idx);
                        ready_types.push_back(DECAF_LINK);
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

        // return index in my_nodes_ of workflow node id
        // -1: not found
        int my_node(int workflow_id)
            {
                for (size_t i = 0; i < my_nodes_.size(); i++)
                {
                    if (my_nodes_[i].idx == workflow_id)
                        return i;
                }
                return -1;
            }

        // return index in my_links_ of workflow link id
        // -1: not found
        int my_link(int workflow_id)
            {
                for (size_t i = 0; i < my_links_.size(); i++)
                {
                    if (my_links_[i].idx == workflow_id)
                        return i;
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
            RoutingNode nl;
            nl.idx  = i;
            nl.done = false;
            nl.wflow_in_links =  set<int>(workflow_.nodes[i].in_links.begin(),
                                          workflow_.nodes[i].in_links.end());
            my_nodes_.push_back(nl);
        }
    }

    // add my links
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(world->rank(), i))
        {
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
           const vector<int>& sources = vector<int>())   // workflow source nodes
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
    int(*node_func)(void*,                       // ptr to producer or consumer callback func
                    vector<Dataflow*>*,
                    vector< shared_ptr<ConstructData> >*);
    int(*dflow_func)(void*,                      // ptr to dataflow callback func in a module
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

        node_func = (int(*)(void*,
                            vector<Dataflow*>*,
                            vector< shared_ptr<ConstructData> >*))
            load_node(mods, workflow_.nodes[i].path, workflow_.nodes[i].func);

        int done = node_func(workflow_.nodes[i].args,
                             &out_dataflows,
                             NULL);
        if (done)
        {
            int my_id = my_node(i);
            if (my_id >= 0)
                my_nodes_[my_id].done = true;
        }
    }

    // remaining (nonsource) tasks and dataflows are driven by receiving messages
    while (1)
    {
        // get incoming data
        for (size_t i = 0; i < link_in_dataflows.size(); i++) // I am a link
        {
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            if (link_in_dataflows[i]->get(container, DECAF_LINK))
                containers.push_back(container);
        }
        for (size_t i = 0; i < node_in_dataflows.size(); i++) // I am a node
        {
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            if (node_in_dataflows[i]->get(container, DECAF_NODE))
                containers.push_back(container);
        }

        // route the message: decide what dataflows and tasks should accept it
        vector<int> ready_ids;                                // index of node or link in workflow
        vector<int> ready_types;                              // type of node or link
        int done = router(containers, ready_ids, ready_types);

        // check for termination and propagate the quit message
        for (list< shared_ptr<ConstructData> >::iterator it = containers.begin();
             it != containers.end(); it++)
        {
            // add quit flag to containers object, initialize to false
            if (Dataflow::test_quit(*it))
            {
                // send quit to destinations
                shared_ptr<ConstructData> quit_container = make_shared<ConstructData>();
                for (size_t i = 0; i < ready_ids.size(); i++)
                {
                    // a workflow node (producer, consumer, or both)
                    Dataflow::set_quit(quit_container);
                    if (ready_types[i] & DECAF_NODE)
                    {
                        for (size_t j = 0; j < workflow_.nodes[ready_ids[i]].out_links.size(); j++)
                            dataflows[workflow_.nodes[ready_ids[i]].out_links[j]]->
                                put(quit_container, DECAF_NODE);
                    }
                    // a workflow link (dataflow)
                    else if (ready_types[i] & DECAF_LINK)
                        dataflows[ready_ids[i]]->put(quit_container, DECAF_LINK);
                }
            }
        }

        // all done
        if (done)
        {
            for (size_t i = 0; i < dataflows.size(); i++)
            {
                if (my_link(i))
                    dataflows[i]->shutdown();
            }
            break;
        }

        // otherwise, act on the messages
        for (size_t i = 0; i < ready_ids.size(); i++)
        {
            // a workflow node (producer, consumer, or both)
            if (ready_types[i] & DECAF_NODE)
            {
                // fill out_dataflows for this node
                out_dataflows.clear();
                for (size_t j = 0; j < workflow_.nodes[ready_ids[i]].out_links.size(); j++)
                    out_dataflows.push_back(dataflows[workflow_.nodes[ready_ids[i]].out_links[j]]);

                // fill node_containers for this node
                vector< shared_ptr<ConstructData> > node_containers; // input messages for 1 node
                for (size_t j = 0; j < workflow_.nodes[ready_ids[i]].in_links.size(); j++)
                {
                    for (list< shared_ptr<ConstructData> >::iterator it = containers.begin();
                         it != containers.end(); it++)
                    {
                        if (link_id(*it) == workflow_.nodes[ready_ids[i]].in_links[j])
                            node_containers.push_back(*it);
                    }
                }

                node_func = (int(*)(void*,             // load the function
                                    vector<Dataflow*>*,
                                    vector< shared_ptr<ConstructData> >*))
                    load_node(mods,
                              workflow_.nodes[ready_ids[i]].path,
                              workflow_.nodes[ready_ids[i]].func);

                int done =  node_func(workflow_.nodes[ready_ids[i]].args, // call it
                                      &out_dataflows,
                                      &node_containers);
                if (done)
                {
                    int my_id = my_node(ready_ids[i]);
                    if (my_id >= 0)
                        my_nodes_[my_id].done = true;
                }

                // remove used messages only after the callback has been called
                for (size_t j = 0; j < workflow_.nodes[ready_ids[i]].in_links.size(); j++)
                {
                    list< shared_ptr<ConstructData> >::iterator it = containers.begin();
                    // using while instead of for: erase inside loop disrupts the iteration
                    while (it != containers.end())
                    {
                        if (link_id(*it) == workflow_.nodes[ready_ids[i]].in_links[j])
                            // erase kills the std iterator but returns the next one
                            it = containers.erase(it);
                        else
                            it++;
                    }
                }
            }

            // a workflow link (dataflow)
            else if (ready_types[i] & DECAF_LINK)
            {
                list< shared_ptr<ConstructData> >::iterator it = containers.begin();
                // using while instead of for: erase inside loop disrupts the iteration
                while (it != containers.end())
                {
                    if (link_id(*it) == ready_ids[i])
                    {
                        dflow_func = (int(*)(void*,            // load the function
                                             Dataflow*,
                                             shared_ptr<ConstructData>))
                            load_dflow(mods,
                                       workflow_.links[ready_ids[i]].path,
                                       workflow_.links[ready_ids[i]].func);

                        int done = dflow_func(workflow_.links[ready_ids[i]].args, // call it
                                              dataflows[ready_ids[i]],
                                              *it);
                        if (done)
                        {
                            int my_id = my_link(ready_ids[i]);
                            if (my_id >= 0)
                                my_links_[my_id].done = true;
                        }
                        // erase kills the std iterator but returns the next one
                        it = containers.erase(it);
                    }
                    else
                        it++;
                }
            }                                           // workflow link
        }                                               // for i = 0; i < ready_ids.size()
        usleep(10);
    }                                                       // while (1)

    // cleanup
    unload_mods(mods);
    for (size_t i = 0; i < dataflows.size(); i++)
        delete dataflows[i];
}

// every user application needs to implement the following run function
void run(Workflow&          workflow,
         const vector<int>& sources = vector<int>());

// pybind11 python bindings
#ifdef PYBIND11

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// PYBIND11_PLUGIN(py_linear_2nodes)
PYBIND11_PLUGIN(pymod)
{
    py::module m("pymod", "pybind11 module");

    py::class_<WorkflowNode>(m, "WorkflowNode")
        .def(py::init<int, int, string, string>())
        .def_readwrite("out_links", &WorkflowNode::out_links)
        .def_readwrite("in_links",  &WorkflowNode::in_links)
        .def("add_out_link", &WorkflowNode::add_out_link)
        .def("add_in_link",  &WorkflowNode::add_in_link);

    py::class_<WorkflowLink>(m, "WorkflowLink")
        .def(py::init<int, int, int, int, string, string, string, string>());

    py::class_<Workflow>(m, "Workflow")
        .def(py::init<vector<WorkflowNode>&, vector<WorkflowLink>&>());

    m.def("run", &run, "Run the workflow",
          py::arg("workflow"), py::arg("sources") = vector<int>());

    return m.ptr();
}

#endif  // PYBIND11

#endif  // DECAF_HPP
