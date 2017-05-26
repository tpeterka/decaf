//---------------------------------------------------------------------------
//
// 2 nodes example for contracts and filtering fields
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
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

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
		SimpleFieldi var(timestep);
		SimpleFieldi dummy(10);

		// Sends two different fields

		// The field var is not sent in the port Out, this raises an error.
		// Uncomment the next line to get rid of the runtime error
		//container->appendData("var", var);

		container->appendData("dummy", dummy);

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
		SimpleFieldi var = in_data.at("In")->getFieldData<SimpleFieldi>("var");
		if(var){
			fprintf(stderr, "Consumer of rank %d received the value %d\n", rank, var.getData());
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
		fprintf(stderr, "The field dummy is %sin the dflow\n", in_data->hasData("dummy")?"" : "not ");

		dataflow->put(in_data, DECAF_LINK);
	}

} // extern "C"


int main(int argc,
         char** argv)
{
	Workflow workflow;
	Workflow::make_wflow_from_json(workflow, "simple_2nodes.json");

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
