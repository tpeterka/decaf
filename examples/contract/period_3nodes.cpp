//---------------------------------------------------------------------------
//
// 3 nodes example for contracts with periodicity of fields
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

using namespace decaf;
using namespace std;

void node1(Decaf* decaf)
{
	int rank = decaf->world->rank();

	// produce data for some number of timesteps
	for (int timestep = 0; timestep < 5; timestep++){
		//fprintf(stderr, "prod rank %d timestep %d\n", rank, timestep);

		pConstructData container;
		SimpleFieldi var(10+timestep);

		container->appendData("var", var,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
							  DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		fprintf(stderr, "\nNode1 it=%d sent %d\n", timestep, 10+timestep);

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
void node2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	int var;
	int timestep = 0;

	vector<pConstructData> in_data;

	SimpleFieldi recv;

	while(decaf->get(in_data)){
		var = 0;
		for(pConstructData data : in_data){
			fprintf(stderr, "Node2 it=%d %sreceived var\n", timestep, data->isEmpty()?"did not " : "");
			if(data->hasData("var")){
				recv = data->getFieldData<SimpleFieldi>("var");
				if(recv){
					var += recv.getData();
				}
			}
		}

		pConstructData d;
		SimpleFieldi field(var);
		d->appendData("var", field);
		if(!decaf->put(d)){
			break;
		}
		timestep++;
		usleep(50000);
	}
	//fprintf(stderr, "prod2 %d terminating\n", rank);
	decaf->terminate();
}

// prod2
void node3(Decaf* decaf)
{
	int rank = decaf->world->rank();
	int var;
	int timestep = 0;

	vector<pConstructData> in_data;

	while(decaf->get(in_data)){
		for(pConstructData data : in_data){
			if(data->hasData("var")){
				var = data->getFieldData<SimpleFieldi>("var").getData();
			}

			fprintf(stderr, "Node3 it=%d received %d\n", timestep, var);
		}

		timestep++;
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
	if (decaf->my_node("node1"))
		node1(decaf);
	if (decaf->my_node("node2"))
		node2(decaf);
	if (decaf->my_node("node3"))
		node3(decaf);
	
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
	Workflow::make_wflow_from_json(workflow, "period_3nodes.json");

	// run decaf
	run(workflow);

	return 0;
}
