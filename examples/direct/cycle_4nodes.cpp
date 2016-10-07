//---------------------------------------------------------------------------
//
// 4-node graph with 2 cycles
//
//       <---------  node_b (1 proc, rank 5)
//      |                                  |
//     dflow (1 proc, rank 10)             |
//      |                                  |
//      |        ->dflow (1 proc, rank 4) ->
//      |       |
//      -> node_a (4 procs, ranks 0-3)
//         |
//         dflow (1 proc, rank 6)
//         |
//         -> node_c (1 proc, rank 7) --> dflow (1 proc, rank 8) --> node_d (1 proc, rank9)
//
//  entire workflow takes 11 procs (1 dataflow proc between each producer consumer pair)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>
#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <utility>
#include <map>

using namespace decaf;
using namespace std;

void node_a(Decaf* decaf)
{
    // produce data for some number of timesteps
    for (int timestep = 0; timestep < 10; timestep++)
    {
        int sum = 0;

        if (timestep > 0)
        {
            // receive data from all inbound dataflows
            // in this example there is only one inbound dataflow,
            // but in general there could be more
            vector< pConstructData > in_data;
            decaf->get(in_data);

            // get the values and add them
            for (size_t i = 0; i < in_data.size(); i++)
            {
                ArrayFieldi field = in_data[i]->getFieldData<ArrayFieldi >("vars");
                if(field)
                    sum += field.getArray()[0];
                else
                    fprintf(stderr, "Error: null pointer in node_a\n");
            }
        }

        fprintf(stderr, "node_a: timestep %d sum = %d\n", timestep, sum);

        // the data in this example is just the timestep, add it to a container
        SimpleFieldi data(timestep);
        pConstructData container;
        container->appendData("var", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_a terminating\n");
    decaf->terminate();
}

void node_b(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data[i]->getFieldData<SimpleFieldi >("var");
            if(field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in node1\n");
        }

        fprintf(stderr, "node_b: sum = %d\n", sum);

        // replicate the sums in an array so that they can be sent to a destination with
        // more ranks
        int* sums = new int[decaf->dataflow(1)->sizes()->con_size];
        for (size_t i = 0; i < decaf->dataflow(1)->sizes()->con_size; i++)
            sums[i] = sum;

        // append the array to a container
        ArrayFieldi data(sums, 4, 1, false);
        pConstructData container;
        container->appendData("vars", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);

        delete[] sums;
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_b terminating\n");
    decaf->terminate();
}

void node_c(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data[i]->getFieldData<SimpleFieldi >("var");
            if(field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in node1\n");
        }

        fprintf(stderr, "node_c: sum = %d\n", sum);

        // append the sum to a container
        SimpleFieldi data(sum);
        pConstructData container;
        container->appendData("var", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_c terminating\n");
    decaf->terminate();
}

void node_d(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data[i]->getFieldData<SimpleFieldi >("var");
            if(field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in node_d\n");
        }

        fprintf(stderr, "node_d: sum = %d\n", sum);

        // add 1 to the sum and send it back (for example)
        sum++;
        SimpleFieldi data(sum);
        pConstructData container;
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_d terminating\n");
    decaf->terminate();
}

// node and link callback functions
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
              Dataflow* dataflow,                  // dataflow
              pConstructData in_data)   // input data
    {
        // fprintf(stderr, "dataflow\n");
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

void run(Workflow& workflow)                     // workflow
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
    if (decaf->my_node("node_a"))
        node_a(decaf);
    if (decaf->my_node("node_b"))
        node_b(decaf);
    if (decaf->my_node("node_c"))
        node_c(decaf);
    if (decaf->my_node("node_d"))
        node_d(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
int main(int argc,
         char** argv)
{
    Workflow workflow;
    /*char * prefix = getenv("DECAF_PREFIX");
    if (prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/mod_cycle_4nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.in_links.push_back(1);                     // node_b
    node.out_links.push_back(3);
    node.start_proc = 5;
    node.nprocs = 1;
    node.func = "node_b";
    workflow.nodes.push_back(node);

    node.out_links.clear();                         // node_d
    node.in_links.clear();
    node.in_links.push_back(0);
    node.start_proc = 9;
    node.nprocs = 1;
    node.func = "node_d";
    workflow.nodes.push_back(node);

    node.out_links.clear();                         // node_c
    node.in_links.clear();
    node.out_links.push_back(0);
    node.in_links.push_back(2);
    node.start_proc = 7;
    node.nprocs = 1;
    node.func = "node_c";
    workflow.nodes.push_back(node);

    node.out_links.clear();                         // node_a
    node.in_links.clear();
    node.out_links.push_back(1);
    node.out_links.push_back(2);
    node.in_links.push_back(3);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "node_a";
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 2;                                  // node_c -> node_d
    link.con = 1;
    link.start_proc = 8;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                                  // node_a -> node_b
    link.con = 0;
    link.start_proc = 4;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                                  // node_a -> node_c
    link.con = 2;
    link.start_proc = 6;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "proc";
    link.dflow_con_redist = "proc";
    workflow.links.push_back(link);

    link.prod = 0;                                  // node_b -> node_a
    link.con = 3;
    link.start_proc = 10;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // identify sources
    vector<int> sources;
    sources.push_back(3);                           // node_a
    */

    Workflow::make_wflow_from_json(workflow, "cycle.json");
    // run decaf
    run(workflow);

    return 0;
}
