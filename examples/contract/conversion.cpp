//---------------------------------------------------------------------------
//
// 3 nodes example for testing contract on a link
//
// clement Mommessin
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/vectorfield.hpp>
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
	int rank = decaf->world->rank();
	vector<int> vect(2);
	// produce data for some number of timesteps
	for (int timestep = 0; timestep <= 6; timestep++){
		fprintf(stderr, "prod rank %d timestep %d\n", rank, timestep);

		vect[0] = 1;
		vect[1] = 2;
		VectorFieldi vf(vect, 1);

		pConstructData container;
		container->appendData("vector", vf,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		if(! decaf->put(container, "Out") ){
			break;
		}
		usleep(200000);
	}
	decaf->terminate();
}

// producer 2
void prod2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	int arr[2];

	for (int timestep = 0; timestep <= 6; timestep++){
		fprintf(stderr, "prod2 rank %d timestep %d\n", rank, timestep);

		arr[0] = 3;
		arr[1] = 4;

		ArrayFieldi af(arr, 2, 1);

		pConstructData container;
		container->appendData("array", af,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

		if(! decaf->put(container, "Out")){
			break;
		}
		usleep(200000);
	}
	//fprintf(stderr, "prod2 %d terminating\n", rank);
	decaf->terminate();
}


// consumer
void con(Decaf* decaf)
{
	int rank = decaf->world->rank();
	map<string, pConstructData> in_data;
	vector<int> vector;

	while (decaf->get(in_data))
	{
		VectorFieldi vect, conv;

		vect = in_data.at("In1")->getFieldData<VectorFieldi>("vector");
		if(!vect){
			cout << "Consumer did not received 'vector' correctly" << endl;
		}

		conv = in_data.at("In2")->getFieldData<VectorFieldi>("converted");
		if(!conv){
			cout << "Consumer did not received 'converted' correctly" << endl;
		}

		vector = vect.getVector();
		vector.insert(vector.end(), conv.getVector().begin(), conv.getVector().end());

		string s = "";
		for(int i : vector){
			s += to_string(i) + " ";
		}
		cout << "Consumer received the values: " << s << endl;
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "con1 %d terminating\n", rank);
	decaf->terminate();
}


// link callback function
extern "C"
{
    // This dataflow converts an Array_int into a Vector_int
    void dflow2(void* args,                          // arguments to the callback
	           Dataflow* dataflow,                  // dataflow
	           pConstructData in_data)   // input data
	{
		vector<int> vect;

		if(in_data->hasData("array")){
			ArrayFieldi af = in_data->getFieldData<ArrayFieldi>("array");
			int size = af.getArraySize();
			int* arr = af.getArray();
			for(int i = 0; i<size; i++){
				vect.push_back(arr[i]);
			}
		}else{
			std::cout << "Link received empty message" << std::endl;
		}

		VectorFieldi vf(vect, 1);
		pConstructData out_data;
		out_data->appendData("converted", vf, DECAF_NOFLAG, DECAF_PRIVATE,
		                     DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		dataflow->put(out_data, DECAF_LINK);
	}

} // extern "C"


// link callback function
extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow1(void* args,                          // arguments to the callback
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
	Workflow::make_wflow_from_json(workflow, "conversion.json");

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

	// cleanup
	delete decaf;
	MPI_Finalize();

	return 0;
}
