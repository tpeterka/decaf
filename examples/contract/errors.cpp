//---------------------------------------------------------------------------
//
// 2 nodes example for testing typechecking of contracts
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
#include <vector>

using namespace decaf;
using namespace std;

void prod(Decaf* decaf)
{
	int rank = decaf->world->rank();

	for (int timestep = 0; timestep < 3; timestep++){
		if(rank == 0){
			fprintf(stderr, "\n----- ITERATION %d -----\n", timestep);
		}
		pConstructData container;


		// --- SCENARIO 3 ---
		// The contract says var is of type Vector_int corresponding to a VectorFieldi
		// Sending var1 gives an error at runtime, sending var2 does not due to typechecking
		int arr[1];
		arr[0] = timestep;
		ArrayFieldi var1(arr, 1, 1);
		container->appendData("var", var1);
		// --- END 3 ---

		/*
		// --- SCENARIO 4 ---
		// Comment the 3 lines above and uncomment these to run correctly
		vector<int> vect(1);
		vect[0] = timestep;
		VectorFieldi var2(vect, 1);
		container->appendData("var", var2);
		// --- END 4 ---
		*/

		if(!decaf->put(container, "Out"))
			break;
		usleep(100000);
	}

	decaf->terminate();
}

void con(Decaf* decaf)
{
	int rank = decaf->world->rank();
	map<string, pConstructData> in_data;

	while(decaf->get(in_data))
	{
		VectorFieldi var = in_data["In"]->getFieldData<VectorFieldi>("var");
		if(var){
			fprintf(stderr, "Consumer received the value of the vector: %d\n", var.getVector()[0]);
		}
	}

	decaf->terminate();
}


// link callback function
extern "C"
{
    // the link just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
	           Dataflow* dataflow,                  // dataflow
	           pConstructData in_data)   // input data
	{
		dataflow->put(in_data, DECAF_LINK);
	}

} // extern "C"


int main(int argc,
         char** argv)
{
	Workflow workflow;
	Workflow::make_wflow_from_json(workflow, "errors.json");

	MPI_Init(NULL, NULL);

	// create decaf
	Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

	// run workflow node tasks
	// decaf simply tells the user whether this rank belongs to a workflow node
	// how the tasks are called is entirely up to the user
	// e.g., if they overlap in rank, it is up to the user to call them in an order that makes
	// sense (threaded, alternating, etc.)
	// also, the user can define any function signature he wants
	if (decaf->my_node("prod"))
		prod(decaf);
	if (decaf->my_node("con"))
		con(decaf);

	// cleanup
	delete decaf;
	MPI_Finalize();

	return 0;
}
