//---------------------------------------------------------------------------
//
// 4 nodes example for testing ports
//
// Clement Mommessin
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
	int rank = decaf->world->rank();
	std::vector<int> vect(3);

	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 5; timestep++){
		if(rank == 0){
			fprintf(stderr, "\n----- ITERATION %d -----\n", timestep);
		}

		vect[0] = rank*100 + std::rand() % 100;
		vect[1] = rank*100 + std::rand() % 100;
		vect[2] = rank*100 + std::rand() % 100;

		pConstructData container;
		VectorFieldi vf(vect, 1);

		SimpleFieldf sf(float(3.14));
		container->appendData("pi", sf);

		container->appendData("value", vf,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
							  DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		// send the data on the given port, the translation to the correct dataflow is done internally
		//if(! decaf->put(container, "Out")){
		if(!decaf->put(container)){
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
	int rank = decaf->world->rank();
	int arr[3];

	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 5; timestep++){

		arr[0] = rank*100 + std::rand() % 100;
		arr[1] = rank*100 + std::rand() % 100;
		arr[2] = rank*100 + std::rand() % 100;

		pConstructData container;
		ArrayFieldi af(arr, 3, 1);

		container->appendData("value", af,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		// send the data on the given port, the translation to the correct dataflow is done internally
		if(! decaf->put(container, "Out")){
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

	map<string, pConstructData> in_data;

	ArrayFieldi af;
	VectorFieldi vf;
	string s;


	while(decaf->get(in_data)){
		s = "";

		//if(!in_data["In1"]->isEmpty()){
			vf = in_data["In1"]->getFieldData<VectorFieldi>("value");
			if(vf){
				vect = vf.getVector();
				for (int i : vect){
					s+= to_string(i) + " ";
				}
				//s+= "Vector size: " + std::to_string(vect.size()) + " ";
			}
		//}
		//if(!in_data["In2"]->isEmpty()){
			af = in_data["In2"]->getFieldData<ArrayFieldi>("value");
			if(af){
				arr= af.getArray();
				for(int i = 0; i<af.getArraySize(); i++){
					s+= to_string(arr[i]) + " ";
				}
				//s+= "Array size: " + std::to_string(af.getArraySize()) + " ";
			}
		//}

		fprintf(stderr, "Consumer rank %d received %s\n", rank, s.c_str());
	}
	decaf->terminate();
}

// Con2
void con2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	float pi;
	pConstructData data;

	SimpleFieldf sf;

	vector<pConstructData> in_data;
	while(decaf->get(in_data)){
		for(pConstructData d : in_data){
			if(d->hasData("pi")){
				sf = d->getFieldData<SimpleFieldf>("pi");
				if(sf){
					pi = sf.getData();
					fprintf(stderr, "I received the value of pi: %.2f!!\n", pi);
				}
				fprintf(stderr, "I%s received the field value!\n", data->hasData("value")?" have" : " have not");
			}
		}
	}

	/*bool ok = true;
	while(ok){
		if(ok = decaf->get(data, "In")){
			sf = data->getFieldData<SimpleFieldf>("pi");
			if(sf){
				pi = sf.getData();
				fprintf(stderr, "I received the value of pi: %.2f!!\n", pi);
			}
			fprintf(stderr, "I%s received the field value!\n", data->hasData("value")?" have" : " have not");
		}

		print

	}*/

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
	if(decaf->my_node("con2"))
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
	Workflow::make_wflow_from_json(workflow, "ports.json");

	// run decaf
	run(workflow);

	return 0;
}
