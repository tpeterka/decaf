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
	std::srand(std::time(0));
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

		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container) ){
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
	std::srand(std::time(0));
	int rank = decaf->world->rank();
	int arr[3];

	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 5; timestep++){

		arr[0] = 100 + std::rand() % 100;
		arr[1] = 100 + std::rand() % 100;
		arr[2] = 100 + std::rand() % 100;

		pConstructData container;
		ArrayFieldi af(arr, 3, 1);

		container->appendData("value2", af,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container) ){
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
	int* arr;
	std::vector<int> vect;

	vector<pConstructData> in_data;
	ArrayFieldi af;
	VectorFieldi vf;

	while(decaf->get(in_data)){
		std::string s = "";
		for(pConstructData data : in_data){
			if(data->hasData("value")){
				vf = data->getFieldData<VectorFieldi>("value");
				if(vf){
					vect = vf.getVector();
					s+= "Vector size: " + std::to_string(vect.size()) + " ";
				}
				else{
					fprintf(stderr, "TOTO\n");
				}
			}
			if(data->hasData("value2")){
				af = data->getFieldData<ArrayFieldi>("value2");
				if(af){
					arr = af.getArray();
					s+= "Array size: " + std::to_string(af.getArraySize()) + " ";
				}
				else{
					fprintf(stderr, "TATA\n");
				}
			}
		}
		fprintf(stderr, "Consumer received %s\n", s.c_str());

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
	if (decaf->my_node("prod2"))
		prod2(decaf);
	if (decaf->my_node("con"))
		con(decaf);
	
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
	Workflow::make_wflow_from_json(workflow, "ports.json");

	// run decaf
	run(workflow);

	return 0;
}
