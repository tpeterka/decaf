//---------------------------------------------------------------------------
//
// 3 nodes example for testing ports and edge_id
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
#include <decaf/data_model/vectorfield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <ctime>

using namespace decaf;
using namespace std;

void prod(Decaf* decaf)
{
	//std::srand(std::time(0));
	std::srand(1);
	int rank = decaf->world->rank();
	std::vector<int> vect(3);

	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 5; timestep++){
		if(rank == 0){
			fprintf(stderr, "\n----- ITERATION %d -----\n", timestep);
		}

		vect[0] = std::rand() % 100;
		vect[1] = std::rand() % 100;
		vect[2] = std::rand() % 100;

		pConstructData container;
		VectorFieldi vf(vect, 3);

		container->appendData("value", vf,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
							  DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		//fprintf(stderr, "Prod sent\n");
		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container, 1) ){
			break;
		}
		usleep(100000);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "prod1 %d terminating\n", rank);
	decaf->terminate();
}

// prod2
void prod2(Decaf* decaf)
{
	//std::srand(std::time(0));
	std::srand(2);
	int rank = decaf->world->rank();
	int arr[3];

	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 5; timestep++){

		arr[0] = 100 + std::rand() % 100;
		arr[1] = 100 + std::rand() % 100;
		arr[2] = 100 + std::rand() % 100;

		pConstructData container;
		ArrayFieldi af(arr, 3, 1);

		container->appendData("value", af,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		//fprintf(stderr, "Prod2 sent\n");
		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container, 0) ){
			break;
		}
		usleep(100000);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "prod1 %d terminating\n", rank);
	decaf->terminate();
}

// con
void con(Decaf* decaf)
{
	int rank = decaf->world->rank();
	std::vector<int> vect;

	map<int, pConstructData> in_data;

	VectorFieldi vf;
	string s;

	while(decaf->get(in_data)){
		s = "";

		vf = in_data.at(1)->getFieldData<VectorFieldi>("value");
		if(vf){
			fprintf(stderr, "Consumer rank %d received Vector\n", rank);
		}
		else{
			fprintf(stderr, "Con rank %d did not received it\n", rank);
		}

	}

	//fprintf(stderr, "prod2 %d terminating\n", rank);
	decaf->terminate();
}


void con2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	int* arr;

	map<int, pConstructData> in_data;

	ArrayFieldi af;
	string s;

	while(decaf->get(in_data)){
		s = "";

		af = in_data.at(0)->getFieldData<ArrayFieldi>("value");
		if(af){
			fprintf(stderr, "Con2 rank %d received Array\n", rank);
		}
	}

	//fprintf(stderr, "prod2 %d terminating\n", rank);
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


// test driver for debugging purposes
// this is hard-coding the no overlap case
int main(int argc,
         char** argv)
{
	Workflow workflow;
	Workflow::make_wflow_from_json(workflow, "ports.json");

	// run decaf
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
	if (decaf->my_node("prod2"))
		prod2(decaf);
	if (decaf->my_node("con"))
		con(decaf);
	if (decaf->my_node("con2"))
		con2(decaf);

	// cleanup
	delete decaf;
	MPI_Finalize();

	return 0;
}
