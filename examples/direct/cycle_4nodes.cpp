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
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/boost_macros.h>

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


        // receive data from all inbound dataflows
        // in this example there is only one inbound dataflow,
        // but in general there could be more
        map<string, pConstructData> in_data;
        decaf->get(in_data);

        pConstructData& inMsg = in_data.at("in");
        if(inMsg->isToken())
            fprintf(stderr, "Reception of a token.\n");
        else
        {
            // get the values and add them
            for (size_t i = 0; i < in_data.size(); i++)
            {
                // Only the first rank will receive a data
                if(inMsg->hasData("vars"))
                {
                    SimpleFieldi field = inMsg->getFieldData<SimpleFieldi >("vars");
                    if(field)
                        sum += field.getData();
                    else
                        fprintf(stderr, "Error: null pointer in node_a\n");
                }
            }

            fprintf(stderr, "node_a: timestep %d sum = %d\n", timestep, sum);
        }

        // the data in this example is just the timestep, add it to a container
        SimpleFieldi data(timestep);
        pConstructData container;
        container->appendData("var", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container, "out");
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_a terminating\n");
    decaf->terminate();
}

void node_b(Decaf* decaf)
{
    map<string, pConstructData> in_data;
    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data.at("in")->getFieldData<SimpleFieldi >("var");
            if(field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in node_b\n");
        }

        fprintf(stderr, "node_b: sum = %d\n", sum);

        // append the array to a container
        SimpleFieldi data(sum);
        pConstructData container;
        container->appendData("vars", data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container, "out");
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_b terminating\n");
    decaf->terminate();
}

void node_c(Decaf* decaf)
{
    map<string, pConstructData> in_data;
    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data.at("in")->getFieldData<SimpleFieldi >("var");
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
        decaf->put(container, "out");
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "node_c terminating\n");
    decaf->terminate();
}

void node_d(Decaf* decaf)
{
    map<string, pConstructData> in_data;
    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data.at("in")->getFieldData<SimpleFieldi >("var");
            if(field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in node_d\n");
        }

        fprintf(stderr, "node_d: sum = %d\n", sum);

        // add 1 to the sum and send it back (for example)
        // Matthieu: end of the graph, we do not need to send the result
        /*sum++;
        SimpleFieldi data(sum);
        pConstructData container;
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        decaf->put(container, "out");*/
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

#ifdef TRANSPORT_CCI
    uint32_t caps	= 0;
    int ret = cci_init(CCI_ABI_VERSION, 0, &caps);
    if (ret)
    {
        fprintf(stderr, "cci_init() failed with %s\n",
            cci_strerror(NULL, (cci_status)ret));
        exit(EXIT_FAILURE);
    }
#endif

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
    fprintf(stderr, "Rank %d finished.\n", decaf->workflow_comm_rank());
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
int main(int argc,
         char** argv)
{
    Workflow workflow;
	Workflow::make_wflow_from_json(workflow, "cycle.json");

	// run decaf
    run(workflow);

    return 0;
}
