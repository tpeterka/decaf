//---------------------------------------------------------------------------
//
// 3 nodes example for testing periodicity
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
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

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
		SimpleFieldi var(timestep+10);

		container->appendData("var", var,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
							  DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		fprintf(stderr, "Node1 it=%d sent %d\n", timestep, timestep+10);

		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container, "Out") ){
			break;
		}
		usleep(200000);
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

	map<string, pConstructData> in_data;

	SimpleFieldi recv;

	bool need_to_break = false;
	while(!need_to_break && decaf->get(in_data)){
		var = 0;

		recv = in_data.at("In")->getFieldData<SimpleFieldi>("var");
		if(recv){
			var += recv.getData();
			fprintf(stderr, "Node2 it=%d received var=%d, forwarding to Node3\n", timestep, recv.getData());
		}

		pConstructData out_data;
		SimpleFieldi field(var);
		out_data->appendData("var", field);

		if(!decaf->put(out_data, "Out")){
			break;
		}
		timestep++;
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

	map<string, pConstructData> in_data;

	while(decaf->get(in_data)){
		if(in_data.at("In")->hasData("var")){
			SimpleFieldi varf =in_data.at("In")->getFieldData<SimpleFieldi>("var");
			if(varf){
				var = varf.getData();
				fprintf(stderr, "Node3 it=%d received %d\n", timestep, var);
			}
			else{
				fprintf(stderr, "Node3 it=%d did not received var", timestep);
			}
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


// test driver for debugging purposes
// this is hard-coding the no overlap case
int main(int argc,
         char** argv)
{
	Workflow workflow;
	Workflow::make_wflow_from_json(workflow, "period_3nodes.json");

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

	return 0;
}
