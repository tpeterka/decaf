#include <decaf/decaf.hpp>

// constructor
decaf::
Decaf::Decaf(CommHandle world_comm,
             Workflow& workflow) :
    world_comm_(world_comm),
    workflow_(workflow)
{
    world = new Comm(world_comm);

    // Env variables are used for the case where not all the tasks
    // share a single MPI_COMM_WORLD. DECAF_WORKFLOW_RANK gives
    // the offset of the local MPI_COMM_WORLD in the global workflow ranking

    // Fetching the global size of the workflow
    const char* env_decaf_size = std::getenv("DECAF_WORKFLOW_SIZE");
    if (env_decaf_size != NULL)
        workflow_size_ = atoi(env_decaf_size);
    else
        workflow_size_ = CommSize(world_comm);

    // Fetching the global rank of the current process
    // The first rank is the global rank in the workflow
    // The world_rank is then the rank of the process in the local communicator
    // plus the rank of the first rank
    const char* env_first_rank = std::getenv("DECAF_WORKFLOW_RANK");
    int global_first_rank = 0;
    if (env_first_rank != NULL)
        global_first_rank = atoi(env_first_rank);
    workflow_rank_ = CommRank(world_comm) + global_first_rank; // my place in the world

    // build routing table
    // routing table is simply vectors of workflow nodes and links that belong to my process

    // add my nodes
    for (size_t i = 0; i < workflow_.nodes.size(); i++)
    {
        if (workflow_.my_node(workflow_rank_, i))
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
        if (workflow_.my_link(workflow_rank_, i))
        {
            RoutingLink nl;
            nl.idx  = i;
            nl.nin  = 0;
            nl.done = false;
            my_links_.push_back(nl);
        }
    }

    // collect all dataflows
    build_dataflows(dataflows);

    // inbound dataflows
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        // I am a link and this dataflow is me
        if (workflow_.my_link(workflow_rank_, i))
            link_in_dataflows.push_back(dataflows[i]);
        // I am a node and this dataflow is an input
        if (workflow_.my_in_link(workflow_rank_, i))
        {
            node_in_dataflows.push_back(pair<Dataflow*, int>(dataflows[i], i));
            if (dataflows[i]->destPort() != "")
            {
                inPortMap.emplace(dataflows[i]->destPort(), dataflows[i]);
            }
        }
    }

     // outbound dataflows
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        // I am a link and this dataflow is me
        // OR
        // I am a node and this dataflow is an output
        if ((workflow_.my_link(workflow_rank_, i)) || (workflow_.my_out_link(workflow_rank_, i)))
        {

            out_dataflows.push_back(dataflows[i]);
        }
    }

    // TODO once we are sure the unique_out_dataflows set is used for the overlapping thing,
    // move this creation of the outPortMap ~5lines above in the "if i am a node"
    for (Dataflow* df : out_dataflows)
    {
        if (df->srcPort() != "")
        {
            if (outPortMap.count(df->srcPort()) == 1)
            {
                outPortMap[df->srcPort()].push_back(df);
            }
            else
            {
                vector<Dataflow*> vect(1, df);
                outPortMap.emplace(df->srcPort(), vect);
            }
        }
    }

    // Adding the Input and Output Port which are not connected
    // to any Dataflows but still declared by the user.
    // WARNING: will not work with overlapping of NODES, currently not supported by Decaf
    for (size_t i = 0; i < workflow_.nodes.size(); i++)
    {
        if (workflow_.my_node(workflow_rank_, i))
        {
            for (string outPortName : workflow_.nodes[i].outports)
            {
                if (outPortMap.count(outPortName) == 0)
                {
                    fprintf(stderr, "WARNING: Output port %s is declared but not connected.\n", outPortName.c_str());
                    vector<Dataflow*> vect;
                    outPortMap.emplace(outPortName, vect);
                }
            }

            for (string inPortName : workflow_.nodes[i].inports)
            {
                if (inPortMap.count(inPortName) == 0)
                {
                    fprintf(stderr, "WARNING: Intput port %s is declared but not connected.\n", inPortName.c_str());
                    inPortMap.emplace(inPortName, nullptr);
                }
            }
        }
    }

    // link ranks that do not overlap nodes need to be started running
    // first eliminate myself if I belong to a node
    /*for (size_t i = 0; i < workflow_.nodes.size(); i++)
        if (workflow_.my_node(workflow_rank_, i)){
            return;
    TODO verify that the following does the same, without a loop */
    if (my_nodes_.size() > 0)
    {
        return;
    }

    // if I don't belong to a node, I must be part of nonoverlapping link; run me in spin mode
    run_links(false);
}

// destructor
decaf::
Decaf::~Decaf()
{
    // TODO: Following crashes on my mac, for the hacc example, in rank 8 (dflow)
    // but not on other machines so far. Will see if it happens in other contexts,
    // or if it is just my outdated software stack -- TP

    // for (size_t i = 0; i < dataflows.size(); i++)
    //     delete dataflows[i];

    delete world;
}

void
decaf::
Decaf::err()
{
    ::all_err(err_);
}


// put a message on all outbound links
bool
decaf::
Decaf::put(pConstructData container)
{
    //bool ret_ok;
    for (Dataflow* df : out_dataflows)
    {
        //for (size_t i = 0; i < out_dataflows.size(); i++){
        df->put(container, DECAF_NODE);
    }

    // link ranks that do overlap this node need to be run in one-time mode
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }
    return true;
}

// put a message on a particular outbound link
// i is the index of the link in the json format of the workflow.
bool
decaf::
Decaf::put(pConstructData container, int i)
{
    dataflows[i]->put(container, DECAF_NODE);

    // TODO remove the loop, with the given i in argument we know whether there is an ovelapping or not
    // link ranks that do overlap this node need to be run in one-time mode
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }
    return true;
}


// put a message on the outbound link(s) associated to the output port given in argument
bool
decaf::
Decaf::put(pConstructData container, string port)
{
    auto it = outPortMap.find(port);
    if (it == outPortMap.end())
    {
        fprintf(stderr, "ERROR: the output port %s is not present or not associated to a Dataflow. Aborting.\n", port.c_str());
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    //bool ret_ok;
    for (Dataflow* out_df : it->second)
    {
        out_df->put(container, DECAF_NODE);

    }
    // TODO remove the loop, with the given port in argument we know whether there is an ovelapping or not
    // link ranks that do overlap this node need to be run in one-time mode
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }

    return true;
}

// get message from inbound link associated to the input port
// returns true = process messages, false = error occured or quit message
bool
decaf::
Decaf::get(pConstructData container, string port)
{
    container->purgeData();
    auto it = inPortMap.find(port);
    if (it == inPortMap.end())
    {
        fprintf(stderr, "ERROR: the input port %s is not present or not associated to a Dataflow. Aborting.\n", port.c_str());
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if (it->second == nullptr)
    {
        // The input port is declared but not connected
        return false;
    }

    if (it->second->isClose())
    {// A quit message has already been received, nothing to get anymore
        return false;
    }



    // TODO remove the loop, with the given port in argument we know whether there is an ovelapping or not
    // link ranks that do overlap this node need to be run in one-time mode, unless
    // the same rank also was a producer node for this link, in which case
    // the link was already run by the producer node during Decaf::put()
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(workflow_rank_, i) && !workflow_.my_out_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }

    //bool ret_ok;
    it->second->get(container, DECAF_NODE);

    if (msgtools::test_quit(container))
    {
        return false;
    }

    return true;
}

// get messages from all input ports
// returns true = process messages, false = error occured or ALL quit messages
// if an input port is closed, the container in the returned map is empty
bool
decaf::
Decaf::get(map<string, pConstructData> &containers)
{
    if (inPortMap.empty())
    {
        fprintf(stderr, "ERROR: Cannot call get on ports, there is no existing input ports. Aborting.\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }
    // TODO verify this
    // link ranks that do overlap this node need to be run in one-time mode, unless
    // the same rank also was a producer node for this link, in which case
    // the link was already run by the producer node during Decaf::put()
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(workflow_rank_, i) && !workflow_.my_out_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }


    containers.clear();
    //bool ret_ok;

    for (std::pair<string, Dataflow*> pair : inPortMap)
    {
        pConstructData container;

        // If the input port is not connected or already closed
        if (pair.second == nullptr || pair.second->isClose())
        {
            containers.emplace(pair.first, container);
        }
        else
        {
            pair.second->get(container, DECAF_NODE);

            containers.emplace(pair.first, container);
        }
    }

    return !allQuit(); // If all in dataflows are quit return false
}


bool
decaf::
Decaf::get(map<int, pConstructData> &containers){
    // TODO verify this
    // link ranks that do overlap this node need to be run in one-time mode, unless
    // the same rank also was a producer node for this link, in which case
    // the link was already run by the producer node during Decaf::put()
    for (size_t i = 0; i < workflow_.links.size(); i++){
        if (workflow_.my_link(workflow_rank_, i) && !workflow_.my_out_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }

    containers.clear();
    //bool ret_ok;

    for (std::pair<Dataflow*, int> pair : node_in_dataflows) // I am a node
    {
        pConstructData container;
        if(pair.first->isClose())
            containers.emplace(pair.second, container);
        else
        {
            pair.first->get(container, DECAF_NODE);

            containers.emplace(pair.second, container);
        }
    }

    return !allQuit(); // If all in dataflows are quit return false

}

// get messages from all inbound links
// returns true = process messages, false = ALL quit received or error occured
bool
decaf::
Decaf::get(vector< pConstructData >& containers)
{
    // TODO verify this
    // link ranks that do overlap this node need to be run in one-time mode, unless
    // the same rank also was a producer node for this link, in which case
    // the link was already run by the producer node during Decaf::put()
    for (size_t i = 0; i < workflow_.links.size(); i++)
    {
        if (workflow_.my_link(workflow_rank_, i) && !workflow_.my_out_link(workflow_rank_, i))
        {
            run_links(true);
            break;
        }
    }

    containers.clear();
    //bool ret_ok;

    /* //TODO remove this; My guess is it is never reached since this get is never called by a link
    for (size_t i = 0; i < link_in_dataflows.size(); i++) // I am a link
    {
        pConstructData container;
        ret_ok = link_in_dataflows[i]->get(container, DECAF_LINK);
        if(!ret_ok){
            terminate();
            return false;
        }
        containers.push_back(container);

    }*/
    //for (size_t i = 0; i < node_in_dataflows.size(); i++) // I am a node
    for (std::pair<Dataflow*, int> pair : node_in_dataflows)
    {
        if (!pair.first->isClose())
        {
            pConstructData container;
            pair.first->get(container, DECAF_NODE);
            containers.push_back(container);
        }
    }

    return !allQuit(); // If all in dataflows are quit return false

}


// Checks if there are still alive input Dataflows for this node
bool
decaf::
Decaf::allQuit()
{
    for (pair<Dataflow*, int> df : node_in_dataflows)
    {
        if (!df.first->isClose())
            return false;
    }
    for (Dataflow* df : link_in_dataflows)
    {
        if (!df->isClose())
            return false;
    }
    return true;
}

// terminate a node task by sending a quit message to the rest of the workflow
void
decaf::
Decaf::terminate()
{
    if (out_dataflows.empty()) return;
    pConstructData quit_container;
    msgtools::set_quit(quit_container);
    put(quit_container);

    // If the nodes has input, we wait that all the connections
    // have received a terminate signal. This maintains the node alive
    // and avoid deadlocks in case of a loop. This way all the nodes in the loop
    // receive a quit message before leaving instead of sending the quit msg and
    // exiting directly
    for (pair<Dataflow*, int> df : node_in_dataflows)
    {
        while (!df.first->isClose())
        {
            pConstructData msg;
            df.first->get(msg, DECAF_NODE);
        }
    }
}

// whether my rank belongs to this workflow node, identified by the name of its func field
bool
decaf::
Decaf::my_node(const char* name)
{
    for (size_t i = 0; i < workflow_.nodes.size(); i++)
    {
        if (workflow_.my_node(workflow_rank_, i) &&
                !strcmp(name, workflow_.nodes[i].func.c_str()))
            return true;
    }
    return false;
}

// return a pointer to a dataflow, identified by its index in the workflow structure
Dataflow*
decaf::
Decaf::dataflow(int i)
{
    return dataflows[i];
}

// Return the total number of dataflows build by this instance of Decaf
unsigned int
decaf::
Decaf::nb_dataflows()
{
    return dataflows.size();
}

Comm*
decaf::
Decaf::prod_comm()
{
    return out_dataflows[0]->prod_comm();
}
Comm*
decaf::
Decaf::con_comm()
{
    return node_in_dataflows[0].first->con_comm();
}




// run any link ranks
void
decaf::
Decaf::run_links(bool run_once)              // spin continuously or run once only
{
    // incoming messages
    list< pConstructData > containers;

    // dynamically loaded modules (plugins)
    void(*dflow_func)(void*,                      // ptr to dataflow callback func in a module
                      Dataflow*,
                      pConstructData);
    map<string, void*> mods;                      // modules dynamically loaded so far

    // links are driven by receiving messages
    while (1)
    {
        // get incoming data
        for (size_t i = 0; i < link_in_dataflows.size(); i++)
        {
            pConstructData container;
            link_in_dataflows[i]->get(container, DECAF_LINK);
            if(container->getMap()->size() > 0)
                containers.push_back(container);
            else
                fprintf(stderr, "ERROR: reception of an empty message\n");
        }

        // route the message: decide what links should accept it
        vector<int> ready_ids;                                // index of node or link in workflow
        vector<int> ready_types;                              // type of node or link
        int done = router(containers, ready_ids, ready_types);

        // check for termination and propagate the quit message
        for (list< pConstructData >::iterator it = containers.begin();
             it != containers.end(); it++)
        {
            // add quit flag to containers object, initialize to false
            if (msgtools::test_quit(*it))
            {
                // send quit to destinations
                pConstructData quit_container;
                msgtools::set_quit(quit_container);

                for (size_t i = 0; i < ready_ids.size(); i++)
                {
                    if (ready_types[i] & DECAF_LINK)
                    {
                        dataflows[ready_ids[i]]->put(quit_container, DECAF_LINK);
                    }
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
            if (ready_types[i] & DECAF_LINK)
            {

                list< pConstructData >::iterator it = containers.begin();
                // using while instead of for: erase inside loop disrupts the iteration
                while (it != containers.end())
                {
                    if (link_id(*it) == ready_ids[i])
                    {
                        dflow_func = (void(*)(void*,            // load the function
                                              Dataflow*,
                                              pConstructData))
                                load_dflow(mods,
                                           workflow_.links[ready_ids[i]].path,
                                workflow_.links[ready_ids[i]].func);

                        dflow_func(workflow_.links[ready_ids[i]].args, // call it
                                dataflows[ready_ids[i]],
                                *it);

                        it = containers.erase(it);
                    }
                    else
                        it++;
                }
            }                                           // workflow link
        }                                               // for i = 0; i < ready_ids.size()

        if (run_once)
            break;
        usleep(10);
    }                                                   // while (1)

    // cleanup
    unload_mods(mods);
}

// debug: print the workflow
void
decaf::
Decaf::print_workflow()
{
    if (workflow_rank_)           // only print from world rank 0
        return;

    fprintf(stderr, "%ld nodes:\n", workflow_.nodes.size());
    for (size_t i = 0; i < workflow_.nodes.size(); i++)
    {
        fprintf(stderr, "node:\n");
        fprintf(stderr, "start_proc %d, nprocs %d\n",
                workflow_.nodes[i].start_proc, workflow_.nodes[i].nprocs);
        fprintf(stderr, "i %ld, out_links.size %ld, in_links.size %ld\n",
                i, workflow_.nodes[i].out_links.size(),
                workflow_.nodes[i].in_links.size());
        fprintf(stderr, "out_links:\n");
        for (size_t j = 0; j < workflow_.nodes[i].out_links.size(); j++)
            fprintf(stderr, "%d\n", workflow_.nodes[i].out_links[j]);
        fprintf(stderr, "in_links:\n");
        for (size_t j = 0; j < workflow_.nodes[i].in_links.size(); j++)
            fprintf(stderr, "%d\n", workflow_.nodes[i].in_links[j]);

    }

    fprintf(stderr, "%ld links:\n", workflow_.links.size());
    for (size_t i = 0; i < workflow_.links.size(); i++)
        fprintf(stderr, "%d -> %d, start_proc %d, nprocs %d\n", workflow_.links[i].prod, workflow_.links[i].con,
                workflow_.links[i].start_proc, workflow_.links[i].nprocs);
}

// builds a vector of dataflows for all links in the workflow
void
decaf::
Decaf::build_dataflows(vector<Dataflow*>& dataflows)
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

        // TODO: emplace constructor
        dataflows.push_back(new Dataflow(world_comm_,
                                         workflow_size_,
                                         workflow_rank_,
                                         decaf_sizes,
                                         prod,
                                         dflow,
                                         con,
                                         workflow_.links[i],
                                         prod_dflow_redist,
                                         dflow_con_redist));


        dataflows[i]->err();
    }
}

// loads a dataflow module (if it has not been loaded) containing a callback function name
// returns a pointer to the function
void*
decaf::
Decaf::load_dflow(
        map<string, void*>& mods,      // (name, handle) of modules dynamically loaded so far
        string mod_name,               // module name (path) to be loaded
        string func_name)              // function name to be called
{
    map<string, void*>::iterator it;              // iterator into the modules
    pair<string, void*> p;                        // (module name, module handle)
    pair<map<string, void*>::iterator, bool> ret; // return value of insertion into mods
    int(*func)(void*,                             // ptr to callback func
               Dataflow*,
               pConstructData);

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
decaf::
Decaf::unload_mods(map<string, void*>& mods) // (name, handle) of modules dynamically loaded so far
{
    map<string, void*>::iterator it;              // iterator into the modules
    for (it = mods.begin(); it != mods.end(); it++)
        dlclose(it->second);
}

// TODO: make the following 5 functions members of ConstructData?

// returns the source type (node, link) of a message
TaskType
decaf::
Decaf::src_type(pConstructData in_data)   // input message
{
    shared_ptr<SimpleConstructData<TaskType> > ptr =
            in_data->getTypedData<SimpleConstructData<TaskType> >("src_type");
    if (ptr)
    {
        return ptr->getData();
    }
    return DECAF_NONE;
}

// returns the workflow link id of a message
// id is the index in the workflow data structure of the link
int
decaf::
Decaf::link_id(pConstructData in_data)   // input message
{
    shared_ptr<SimpleConstructData<int> > ptr = in_data->getTypedData<SimpleConstructData<int> >("link_id");
    if (ptr)
    {
        return ptr->getData();
    }
    return -1;
}

// returns the workflow destination id of a message
// destination could be a node or link depending on the source type
// id is the index in the workflow data structure of the destination entity
int
decaf::
Decaf::dest_id(pConstructData in_data)   // input message
{
    shared_ptr<SimpleConstructData<int> > ptr = in_data->getTypedData<SimpleConstructData<int> >("dest_id");
    if (ptr)
    {
        return ptr->getData();
    }
    return -1;
}

// routes input data to a callback function
// returns 0 on success, -1 if all my nodes and links are done; ie, shutdown
int
decaf::
Decaf::router(list< pConstructData >& in_data, // input messages
              vector<int>& ready_ids,                  // (output) ready workflow node or link ids
              vector<int>& ready_types)                // (output) ready node or link types
// DECAF_NODE | DECAF_LINK
{
    for (list< pConstructData >::iterator it = in_data.begin();
         it != in_data.end(); it++)
    {
        // is destination a workflow node or a link
        int dest_node = -1;
        int dest_link = -1;

        if (src_type(*it) & DECAF_NODE)
            dest_link = dest_id(*it);
        else if (src_type(*it) & DECAF_LINK)
            dest_node = dest_id(*it);
        else if (src_type(*it) == DECAF_NONE)
        {
            fprintf(stderr, "[%i] ERROR: reception of a none source msg.\n", workflow_rank_);
            fprintf(stderr, "[%i] ERROR: number of fields: %u.\n", workflow_rank_, (*it)->getNbFields());
            (*it)->printKeys();
            shared_ptr<SimpleConstructData<TaskType> > ptr =
                    (*it)->getTypedData<SimpleConstructData<TaskType> >("src_type");
            if (ptr)
            {
                fprintf(stderr, "Value for the src_type: %d\n", ptr->getData());
            }
            else
                fprintf(stderr, "ERROR: the access of the src_type failed.\n");
        }

        if (dest_node >= 0)               // destination is a node
        {
            if (msgtools::test_quit(*it)) // done
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
            if (msgtools::test_quit(*it)) // done
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
        }
        // link
    }                                     // in_data iterator

    // check for ready nodes and links having all their inputs
    ready_nodes_links(ready_ids, ready_types);

    return(all_done());
}                                      // router

// Check for producers/consumers with all their inputs
// check for dataflows with inputs (each dataflow has only one)
// and fill ready_ids and ready_types with the ready nodes and links
void
decaf::
Decaf::ready_nodes_links(vector<int>& ready_ids,      // (output) ready workflow node or link ids
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
int
decaf::
Decaf::all_done()
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
int
decaf::
Decaf::my_node(int workflow_id)
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
int
decaf::
Decaf::my_link(int workflow_id)
{
    for (size_t i = 0; i < my_links_.size(); i++)
    {
        if (my_links_[i].idx == workflow_id)
            return i;
    }
    return -1;
}

// Clear the buffers of the output dataflows
void
decaf::
Decaf::clearBuffers(TaskType role)
{
    for (unsigned int i = 0; i < out_dataflows.size(); i++)
        out_dataflows[i]->clearBuffers(role);
}

CommHandle
decaf::
Decaf::prod_comm_handle()
{
    if (!out_dataflows.empty())
        return out_dataflows[0]->prod_comm_handle();
    else
        return world_comm_; // The task is the only one in the graph
}

CommHandle
decaf::
Decaf::con_comm_handle()
{
    if (!node_in_dataflows.empty())
        return node_in_dataflows[0].first->con_comm_handle();
    else
        return world_comm_; // The task is the only one in the graph
}

int
decaf::
Decaf::prod_comm_size()
{
    if (!out_dataflows.empty())
        return out_dataflows[0]->sizes()->prod_size;
    else // The task is the only one in the graph
    {
        int size_comm;
        MPI_Comm_size(world_comm_, &size_comm);
        return size_comm;
    }
}

int
decaf::
Decaf::con_comm_size()
{
    if (!node_in_dataflows.empty())
        return node_in_dataflows[0].first->sizes()->con_size;
    else // The task is the only one in the graph
    {
        int size_comm;
        MPI_Comm_rank(world_comm_, &size_comm);
        return size_comm;
    }
}

int
decaf::
Decaf::local_comm_size()
{
    // We are the consumer in the inbound dataflow
    if (!node_in_dataflows.empty())
        return node_in_dataflows[0].first->sizes()->con_size;
    else if(!link_in_dataflows.empty())
        return link_in_dataflows[0]->sizes()->dflow_size;
    // We are the producer in the outbound dataflow
    else if(!out_dataflows.empty())
        return out_dataflows[0]->sizes()->prod_size;

    else
    {
        // We don't have nor inbound not outbound dataflows
        // The task is alone in the graph, returning world comm
        int comm_size;
        MPI_Comm_size(world_comm_, &comm_size);
        return comm_size;
    }


}

CommHandle
decaf::
Decaf::local_comm_handle()
{
    // We are the consumer in the inbound dataflow
    if (!node_in_dataflows.empty())
        return node_in_dataflows[0].first->con_comm_handle();
    else if (!link_in_dataflows.empty())
        return link_in_dataflows[0]->dflow_comm_handle();
    // We are the producer in the outbound dataflow
    else if (!out_dataflows.empty())
        return out_dataflows[0]->prod_comm_handle();

    else
    {
        // We don't have nor inbound not outbound dataflows
        // The task is alone in the graph, returning world comm
        return world_comm_;
    }
}

int
decaf::
Decaf::local_comm_rank()
{
    int rank;
    MPI_Comm_rank(this->local_comm_handle(), &rank);
    return rank;
}

int
decaf::
Decaf::prod_comm_size(int i)
{
    if (node_in_dataflows.size() > i)
        return node_in_dataflows[i].first->sizes()->prod_size;
    else if (link_in_dataflows.size() > i)
        return link_in_dataflows[i]->sizes()->prod_size;
    return 0;
}

int
decaf::
Decaf::con_comm_size(int i)
{
    if (out_dataflows.size() > i)
        return out_dataflows[i]->sizes()->con_size;

    return 0;
}

int
decaf::
Decaf::workflow_comm_size()
{
   return workflow_size_;
}

int
decaf::
Decaf::workflow_comm_rank()
{
    return workflow_rank_;
}

