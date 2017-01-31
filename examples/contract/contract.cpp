//---------------------------------------------------------------------------
//
// 4 nodes example for contracts
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
		//fprintf(stderr, "prod rank %d timestep %d\n", rank, timestep);
		for(int i = 0; i<3; ++i){
			array[i] = (i+1)*timestep*(rank+1)*pi;
		}

		SimpleFieldi d_index(rank);
		ArrayFieldf d_velocity(array,3, 3);

		pConstructData container;
		container->appendData("index", d_index,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		container->appendData("velocity", d_velocity,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
							  DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container) ){
			break;
		}
		usleep(50000);
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
		//fprintf(stderr, "prod2 rank %d timestep %d\n", rank, timestep);

		for(int i = 0; i<3; ++i){
			vel_array[i] = timestep*vel*(i+1);
			den_array[i] = (rank+1)*2*timestep*den*(i+1);
		}

		ArrayFieldf d_vel(vel_array, 3, 3);
		ArrayFieldf d_density(den_array, 3, 3);
		SimpleFieldi d_id(rank);

		pConstructData container;
		container->appendData("id", d_id);
		container->appendData("density", d_density,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

		//Data vel is produced but not used by the consumers in this example
		container->appendData("vel", d_vel,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

		if(! decaf->put(container) ){
			break;
		}
		usleep(50000);
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
		float *density, *velocity;
		ArrayFieldf a_density, a_velocity;

		int link_id;
		// retrieve the values get
		for (size_t i = 0; i < in_data.size(); i++)
		{
			link_id = in_data[i]->getFieldData<SimpleFieldi>("link_id").getData();

			if(link_id == 0){
				cout << "link_id 0 and has index: " << in_data[i]->hasData("index") << endl;
			}
			else if(link_id == 2){
				cout << "link_id 2 and has index: " << in_data[i]->hasData("density") << endl;
			}
			else{
				in_data[i]->printKeys();
			}

			if(in_data[i]->hasData("index")){
				index = in_data[i]->getFieldData<SimpleFieldi >("index").getData();
			}
			if(in_data[i]->hasData("velocity")){
				a_velocity.reset();
				a_velocity = in_data[i]->getFieldData<ArrayFieldf>("velocity");
			}
			if(in_data[i]->hasData("density")){
				a_density.reset();
				a_density = in_data[i]->getFieldData<ArrayFieldf>("density");
			}

		}

		fprintf(stderr, "con rank %d received: index %d velocity size %d and density size %d\n", rank, index, a_velocity.getArraySize(), a_density.getArraySize());
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
		ArrayFieldf a_velocity;

		// retrieve the values get
		for (size_t i = 0; i < in_data.size(); i++)
		{
			if(in_data[i]->hasData("id")){
				id = in_data[i]->getFieldData<SimpleFieldi >("id").getData();
			}
			if(in_data[i]->hasData("velocity")){
				a_velocity.reset();
				a_velocity = in_data[i]->getFieldData<ArrayFieldf>("velocity");
			}
		}

		fprintf(stderr, "con2 rank %d received: id %d velocity size %d\n", rank, id, a_velocity.getArraySize());
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
		//fprintf(stderr, "Forwarding data in dflow, having %d fields\n", in_data->getNbFields()-2); //-2 because there are 2 fields used for the system
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
	Workflow::make_wflow_from_json(workflow, "contract.json");

	// run decaf
	run(workflow);

	return 0;
}
