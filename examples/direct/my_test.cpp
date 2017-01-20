//---------------------------------------------------------------------------
//
// 4 node example for contracts
//
// prod (2 procs) - con (2 procs)
// prod2 (2 procs) - con2 (2procs)
//
// entire workflow takes 16 procs (8 dataflows in total)
//
// clement Mommessin
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/boost_macros.h>

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
	int rank = decaf->world->rank();
	float pi = 3.1415;
	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 3; timestep++)
	{
		fprintf(stderr, "prod1 rank %d timestep %d\n", rank, timestep);

		SimpleFieldi d_index(rank);
		SimpleFieldf d_velocity(timestep*rank*pi);

		pConstructData container;
		container->appendData("index", d_index,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		container->appendData("velocity", d_velocity,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		// send the data on all outbound dataflows
		// in this example there is only one outbound dataflow, but in general there could be more
		decaf->put(container);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	fprintf(stderr, "prod1 %d terminating\n", rank);
	decaf->terminate();
}

// prod2
void prod2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	float den = 2.71828;
	float vel = 1.618;

	for (int timestep = 0; timestep < 3; timestep++){
		fprintf(stderr, "prod2 %d timestep %d\n", rank, timestep);

		SimpleFieldf d_vel(timestep*vel);
		SimpleFieldf d_density(rank*2*timestep*den);
		SimpleFieldi d_id(rank);

		pConstructData container;
		container->appendData("id", d_id);
		container->appendData("vel", d_vel,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
		container->appendData("density", d_density,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		decaf->put(container);
	}

	fprintf(stderr, "prod2 %d terminating\n", rank);
	decaf->terminate();
}

// consumer
void con(Decaf* decaf)
{
	int rank = decaf->world->rank();
	vector< pConstructData > in_data;

	while (decaf->get(in_data))
	{
		int index = 0;
		float sum_velocity = 0;
		float sum_density = 0;

		fprintf(stderr, "Con1 %d size of in_data is %lu\n", rank, in_data.size());


		// get the values and add them
		for (size_t i = 0; i < in_data.size(); i++)
		{
			if(in_data[i]->hasData("index")){
				index += in_data[i]->getFieldData<SimpleFieldi >("index").getData();
				fprintf(stderr, "Con1 %d got index %d at i %lu\n", rank, index, i);
			}


		}
		//fprintf(stderr, "con1 %d velocity %f and density %f\n", sum_velocity, sum_density);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	fprintf(stderr, "con1 %d terminating\n", rank);
	decaf->terminate();
}

// con2
void con2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	vector< pConstructData > in_data;

	while (decaf->get(in_data))
	{
		int id = 0;
		float sum_velocity = 0;

		fprintf(stderr, "Con2 %d size of in_data is %lu\n", rank, in_data.size());

		// get the values and add them
		for (size_t i = 0; i < in_data.size(); i++)
		{
			if(in_data[i]->hasData("id")){
				id += in_data[i]->getFieldData<SimpleFieldi >("id").getData();
				fprintf(stderr, "Con2 %d got id %d at i %lu\n", rank, id, i);
			}

		}
		//fprintf(stderr, "con1 %d velocity %f and density %f\n", sum_velocity, sum_density);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	fprintf(stderr, "con1 %d terminating\n", rank);
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
		fprintf(stderr, "Forwarding data in dflow, having %d fields\n", in_data->getNbFields());
		dataflow->put(in_data, DECAF_LINK);
	}
} // extern "C"

void run(Workflow& workflow)                             // workflow
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
	if (decaf->my_node("prod"))
		prod(decaf);
	if (decaf->my_node("con"))
		con(decaf);
	if (decaf->my_node("prod2"))
		prod2(decaf);
	if (decaf->my_node("con2"))
		con2(decaf);
	
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
	Workflow::make_wflow_from_json(workflow, "my_test.json");

	// run decaf
	run(workflow);

	return 0;
}
