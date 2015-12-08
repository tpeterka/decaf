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
    class Decaf
    {
    public:
        Decaf(CommHandle world_comm,
              Workflow& workflow,
              int prod_nsteps) :
            world_comm_(world_comm),
            workflow_(workflow),
            prod_nsteps_(prod_nsteps) { world = new Comm(world_comm); }
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
                            int,
                            int,
                            vector<Dataflow*>*,
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
                            int,
                            int,
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

        // routes input data to a callback function
        void
        router(vector< shared_ptr<ConstructData> > in_data, // input messages
               vector<int> node_link_ids,                   // (output) workflow node or link ids
               vector<int> types)                           // (output) callback types
                                                            // DECAF_PROD, DECAF_DFLOW, DECAF_CON,
                                                            // DECAF_BOTH; DECAF_NONE if no match
            {
                // TODO: tag messages w/ destination id

                // TODO: get workflow as input and parse it to create router table
                // 1st time only in decaf constructor

                // TODO: hard coded some result for now
                // need a lookup table of message_types -> funcs
                node_link_ids.push_back(0);                // node_b in this example
                types.push_back(DECAF_CON);

                // TODO: check whether this process participates in this function?
                // should never fail, a message sent to this process should be for one of its funcs

                // TODO: return DECAF_NONE if no subscribers
            }

        // data members
        CommHandle world_comm_;     // handle to original world communicator
        Workflow workflow_;         // workflow
        int prod_nsteps_;           // total number of producer time steps
        int err_;                   // last error

    };
} // namespace

// runs the workflow
void
decaf::
Decaf::run(void (*pipeliner)(decaf::Dataflow*),    // custom pipeliner code
           void (*checker)(decaf::Dataflow*),      // custom resilience code
           vector<int>& sources)                   // workflow source nodes
{
    // collect dataflows
    vector<decaf::Dataflow*> dataflows;            // dataflows for all links in the workflow
    build_dataflows(dataflows, pipeliner, checker);
    vector<decaf::Dataflow*> out_dataflows;        // outbound dataflows for one node
    vector<decaf::Dataflow*> in_dataflows;         // all inbound dataflows for this process
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (world->rank() >= workflow_.nodes[i].start_proc &&
            world->rank() <  workflow_.nodes[i].start_proc + workflow_.nodes[i].nprocs)
            in_dataflows.push_back(dataflows[i]);
    }

    // dynamically loaded modules (plugins)
    void(*node_func)(void*,                       // ptr to producer or consumer callback func
                     int,                         // in a module
                     int,
                     vector<Dataflow*>*,
                     shared_ptr<ConstructData>);
    void(*dflow_func)(void*,                      // ptr to dataflow callback func in a module
                      int,
                      int,
                      Dataflow*,
                      shared_ptr<ConstructData>);
    map<string, void*> mods;                      // modules dynamically loaded so far

    // main time step loop
    // TODO: This won't work; user must add time step to data model
    for (int t = 0; t < prod_nsteps_; t++)
    {
        // sources, ie, tasks that don't need to wait for input on first step
        // and that run a fixed number of time steps
        for (size_t s = 0; s < sources.size(); s++)
        {
            int i = sources[s];

            // check if this process belongs to the node
            if (world->rank() < workflow_.nodes[i].start_proc ||
                world->rank() >= workflow_.nodes[i].start_proc + workflow_.nodes[i].nprocs)
                continue;

            // fprintf(stderr, "3: world_rank %d start %d end %d\n",
            //         world->rank(), workflow_.nodes[i].start_proc,
            //         workflow_.nodes[i].start_proc + workflow_.nodes[i].nprocs - 1);

            // fill out_dataflows for this node
            for (size_t j = 0; j < workflow_.nodes[i].out_links.size(); j++)
                out_dataflows.push_back(dataflows[workflow_.nodes[i].out_links[j]]);

            node_func = (void(*)(void*,
                                 int,
                                 int,
                                 vector<Dataflow*>*,
                                 shared_ptr<ConstructData>))
                load_node(mods, workflow_.nodes[i].path, workflow_.nodes[i].func);

            node_func(workflow_.nodes[i].args,
                      t,
                      prod_nsteps_,
                      &out_dataflows,
                      NULL);
        }

        // remaining (nonsource) tasks and dataflows are driven by receiving messages
        while (1)
        {
            // get incoming data
            // TODO: need a nonblocking get or probe
            vector< shared_ptr<ConstructData> > containers(in_dataflows.size());
            for (size_t i = 0; i < containers.size(); i++)
                containers[i] = make_shared<ConstructData>();
            for (size_t i = 0; i < in_dataflows.size(); i++)
            {
                // TODO DECAF_CON? (need to also get DFLOW)
                in_dataflows[i]->get(containers[i], DECAF_CON);
            }

            // decide what dataflows and tasks should accept it
            // lookup table of subscribers limited to this process
            // (further filtering on rank unneeded)
            vector<int> nls;                                // index of node or link in workflow
            vector<int> types;                              // type of node or link
            router(containers, nls, types);

            // check for termination
            // if so, remove the task or dataflow from the list
            // TODO

            // TODO how to propagate termination through the task or dataflow?
            // need to call the task or dataflow with a special flag, or should decaf send the
            // terminate further down?

            for (size_t i = 0; i < nls.size(); i++)
            {
                // a workflow node (producer, consumer, or both)
                if (types[i] == DECAF_CON || types[i] == DECAF_PROD || types[i] == DECAF_BOTH)
                {
                    // fill out_dataflows for this node
                    out_dataflows.clear();
                    for (size_t j = 0; j < workflow_.nodes[nls[i]].out_links.size(); j++)
                        out_dataflows.push_back(dataflows[workflow_.nodes[nls[i]].out_links[j]]);

                    node_func = (void(*)(void*,             // load the function
                                         int,
                                         int,
                                         vector<Dataflow*>*,
                                         shared_ptr<ConstructData>))
                        load_node(mods,
                                  workflow_.nodes[nls[i]].path,
                                  workflow_.nodes[nls[i]].func);

                    node_func(workflow_.nodes[nls[i]].args, // call it
                              t,
                              prod_nsteps_,
                              &out_dataflows,
                              containers[i]);
                }

                // a workflow link (dataflow)
                else if (types[i] == DECAF_DFLOW)
                {
                    dflow_func = (void(*)(void*,            // load the function
                                          int,
                                          int,
                                          Dataflow*,
                                          shared_ptr<ConstructData>))
                        load_dflow(mods,
                                   workflow_.links[nls[i]].path,
                                   workflow_.links[nls[i]].func);

                    dflow_func(workflow_.links[nls[i]].args, // call it
                               t,
                               prod_nsteps_,
                               dataflows[nls[i]],
                               containers[i]);
                }
            }

            // TODO: will terminate too early
            if (!nls.size())
                break;
        }                                                   // while (1)
    }                                                       // main time step loop

    // cleanup
    unload_mods(mods);
    for (size_t i = 0; i < dataflows.size(); i++)
        delete dataflows[i];
}

#endif
