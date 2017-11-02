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
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>

#include <bredala/data_model/boost_macros.h>

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
    fprintf(stderr, "node0 terminating\n");
    decaf->terminate();
}

// intermediate node is both a consumer and producer
void node1(Decaf* decaf)
{
    map<string, pConstructData> in_data;
    while (decaf->get(in_data))
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            SimpleFieldi field = in_data.at("in")->getFieldData<SimpleFieldi>("var");
            if(field)
                sum += field.getData();
            else
                fprintf(stderr, "Error: null pointer in node1\n");
        }

        fprintf(stderr, "node1: sum = %d\n", sum);

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
    fprintf(stderr, "node1 terminating\n");
    decaf->terminate();
}

// consumer
void node2(Decaf* decaf)
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
               pConstructData in_data)   // input data
    {
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

void run(Workflow& workflow)                         // workflow
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
int main(int argc,
         char** argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "linear3.json");

    // run decaf
    run(workflow);

    return 0;
}
