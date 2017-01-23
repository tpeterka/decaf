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
#include <decaf/data_model/arrayfield.hpp>
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
	float array[3];
	// produce data for some number of timesteps
	for (int timestep = 1; timestep <4; timestep++){
		//fprintf(stderr, "prod1 rank %d timestep %d\n", rank, timestep);
		for(int i = 0; i<3; ++i){
			array[i] = (i+1)*timestep*(rank+1)*pi;
		}

		SimpleFieldi d_index(rank);
		ArrayFieldf d_velocity(array,3, 1);

		pConstructData container;
		container->appendData("index", d_index,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		container->appendData("velocity", d_velocity,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		decaf->put(container);
		fprintf(stderr, "prod rank %d sent %d fields\n", rank, container->getNbFields());
		sleep(1);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "prod1 %d terminating\n", rank);
	decaf->terminate();
}

// prod2
void prod2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	float den = 2.71828;
	float vel = 1.618;
	float vel_array[3], den_array[3];

	for (int timestep = 1; timestep < 4; timestep++){
		//fprintf(stderr, "prod2 %d timestep %d\n", rank, timestep);

		for(int i = 0; i<3; ++i){
			vel_array[i] = timestep*vel*(i+1);
			den_array[i] = (rank+1)*2*timestep*den*(i+1);
		}


		ArrayFieldf d_vel(vel_array, 3, 3);
		ArrayFieldf d_density(den_array, 3, 3);
		SimpleFieldi d_id(rank);

		pConstructData container;
		container->appendData("id", d_id);
		container->appendData("vel", d_vel,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
		container->appendData("density", d_density,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

		decaf->put(container);
		fprintf(stderr, "prod2 rank %d sent %d fields\n", rank, container->getNbFields());
		sleep(1);
	}

	//fprintf(stderr, "prod2 %d terminating\n", rank);
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
		float* velocity;
		float* density;

		// retrieve the values get
		for (size_t i = 0; i < in_data.size(); i++)
		{
			if(in_data[i]->hasData("index")){
				index = in_data[i]->getFieldData<SimpleFieldi >("index").getData();
			}
			if(in_data[i]->hasData("velocity")){
				velocity = in_data[i]->getFieldData<ArrayFieldf>("velocity").getArray();
			}
			if(in_data[i]->hasData("density")){
				density = in_data[i]->getFieldData<ArrayFieldf>("density").getArray();
			}
		}
		//fprintf(stderr, "con rank %d received index %d velocity %f and density %f\n", rank, index, velocity[1], density[1]);
		fprintf(stderr, "con rank %d received velocities: %f %f %f\n", rank, velocity[0], velocity[1], velocity[2]);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "con1 %d terminating\n", rank);
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
		float* velocity;

		// retrieve the values get
		for (size_t i = 0; i < in_data.size(); i++)
		{
			if(in_data[i]->hasData("id")){
				id = in_data[i]->getFieldData<SimpleFieldi >("id").getData();
			}
			if(in_data[i]->hasData("velocity")){
				velocity = in_data[i]->getFieldData<ArrayFieldf >("velocity").getArray();
			}
		}
		//fprintf(stderr, "con2 rank %d id %d and velocity %f\n", rank, id, velocity[1]);
		fprintf(stderr, "con2 rank %d received velocities: %f %f %f\n", rank, velocity[0], velocity[1], velocity[2]);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "con1 %d terminating\n", rank);
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
		//fprintf(stderr, "Forwarding data in dflow, having %d fields\n", in_data->getNbFields());
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
