﻿//---------------------------------------------------------------------------
//
// 2 nodes example for testing contract on a link
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
#include <cmath>

using namespace decaf;
using namespace std;

void prod(Decaf* decaf)
{
	int rank = decaf->world->rank();
	float val = 0;
	std::srand(std::time(0));

	for (int timestep = 0; timestep < 5; timestep++){
		pConstructData container;

		val = ((double) std::rand() / RAND_MAX);

		SimpleFieldf var(val);

		container->appendData("var", var);

		SimpleFieldi toto(23);
		container->appendData("toto", toto);

		if(!decaf->put(container, "Out"))
			break;
		fprintf(stderr, "Prod %d sent var=%f\n", rank, val);
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

		std::cout << "Con " << rank << " received toto? " << in_data.at("In")->hasData("toto") << std::endl;
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

		int new_var;
		float var = -1;
		SimpleFieldf varf = in_data->getFieldData<SimpleFieldf>("var");
		if(varf){
			var = varf.getData();
		}
		std::cout << "Toto present in link? " << in_data->hasData("toto") << std::endl;

		new_var = std::round(var);
		SimpleFieldi vari(new_var);
		in_data->removeData("var");
		in_data->appendData("var", vari);

		dataflow->put(in_data, DECAF_LINK);
	}

} // extern "C"


int main(int argc,
         char** argv)
{
	Workflow workflow;
	Workflow::make_wflow_from_json(workflow, "contract_link.json");

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
