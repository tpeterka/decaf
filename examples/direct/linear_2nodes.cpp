//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
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
#ifdef TRANSPORT_CCI
#include <cci.h>
#endif
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

using namespace decaf;
using namespace std;

// producer
void prod(Decaf* decaf)
{
    // produce data for some number of timesteps
	for (int timestep = 0; timestep < 10; timestep++)
    {
		fprintf(stderr, "producer %d timestep %d\n", decaf->world->rank(), timestep);

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
	fprintf(stderr, "producer %d terminating\n", decaf->world->rank());
    decaf->terminate();
}

// consumer
void con(Decaf* decaf)
{
    map<string, pConstructData> in_data;
    while (decaf->get(in_data))
    {
        int sum = -1;

        // get the values and add them
        SimpleFieldi field = in_data.at("in")->getFieldData<SimpleFieldi>("var");
        if (field)
            sum = field.getData();
        else
            fprintf(stderr, "Error: null pointer in con\n");


		fprintf(stderr, "consumer %d sum = %d\n", decaf->world->rank(), sum);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
	fprintf(stderr, "consumer %d terminating\n", decaf->world->rank());
    decaf->terminate();
}

// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)   // input data
    {
		int var = in_data->getFieldData<SimpleFieldi>("var").getData();
		fprintf(stderr, "Forwarding data %d in dflow\n", var);
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

void run(Workflow& workflow)                             // workflow
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
    if (decaf->my_node("prod"))
        prod(decaf);
    if (decaf->my_node("con"))
        con(decaf);

    // cleanup
    //delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// this is hard-coding the no overlap case
int main(int argc,
         char** argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "linear2.json");

    // run decaf
    run(workflow);

    return 0;
}
