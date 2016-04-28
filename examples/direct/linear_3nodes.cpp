//---------------------------------------------------------------------------
//
// 3-node linear coupling example
//
// node0 (4 procs) - node1 (2 procs) - node2 (1 proc)
// node0[0,3] -> dflow[4,6] -> node1[7,8] -> dflow[9,10] -> node2[11]
//
// entire workflow takes 12 procs (3 dataflow procs between node0 and node1 and
// 2 dataflow procs between node1 and node2)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>

#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

using namespace decaf;
using namespace std;

// producer
void node0(Decaf* decaf)
{
    // produce data for some number of timesteps
    for (int timestep = 0; timestep < 10; timestep++)
    {
        fprintf(stderr, "node0 timestep %d\n", timestep);

        // the data in this example is just the timestep; add it to a container
        shared_ptr<SimpleConstructData<int> > data =
            make_shared<SimpleConstructData<int> >(timestep);
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node0 terminating\n");
    decaf->terminate();
}

// intermediate node is both a consumer and producer
void node1(Decaf* decaf)
{
    vector< shared_ptr<ConstructData> > in_data;

    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = in_data[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
            }
            else
                fprintf(stderr, "Error: null pointer in node1\n");
        }

        fprintf(stderr, "node1: sum = %d\n", sum);

        // append the sum to a container
        shared_ptr<SimpleConstructData<int> > data  = make_shared<SimpleConstructData<int> >(sum);
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node1 terminating\n");
    decaf->terminate();
}

// consumer
void node2(Decaf* decaf)
{
    vector< shared_ptr<ConstructData> > in_data;

    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = in_data[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
            }
            else
                fprintf(stderr, "Error: null pointer in node2\n");
        }
        fprintf(stderr, "node2 sum = %d\n", sum);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node2 terminating\n");
    decaf->terminate();
}

extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               shared_ptr<ConstructData> in_data)   // input data
    {
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                         // workflow
{
    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run workflow node tasks
    // decaf simply tells the user whether this rank belongs to a workflow node
    // how the tasks are called is entirely up to the user
    // e.g., if they overlap in rank, it is up to the user to call them in an order that makes
    // sense (threaded, alternting, etc.)
    // also, the user can define any function signature she wants
    if (decaf->my_node("node0"))
        node0(decaf);
    if (decaf->my_node("node1"))
        node1(decaf);
    if (decaf->my_node("node2"))
        node2(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// this is hard-coding the no overlap case
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    Workflow workflow;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }

    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/mod_linear_3nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                        // node0
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "node0";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // node1
    node.in_links.clear();
    node.in_links.push_back(0);
    node.out_links.push_back(1);
    node.start_proc = 7;
    node.nprocs = 2;
    node.func = "node1";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // node2
    node.in_links.clear();
    node.in_links.push_back(1);
    node.start_proc = 11;
    node.nprocs = 1;
    node.func = "node2";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 0;                               // dataflow between node0 and node1
    link.con = 1;
    link.start_proc = 4;
    link.nprocs = 3;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 1;                               // dataflow between node1 and node2
    link.con = 2;
    link.start_proc = 9;
    link.nprocs = 2;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow);

    return 0;
}
