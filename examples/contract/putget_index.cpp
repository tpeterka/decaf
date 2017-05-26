//---------------------------------------------------------------------------
//
// 4 nodes example for using indexes of dataflows
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
	float pi = 3.1415;
	float array[3];
	bool ok = true;
	// produce data for some number of timesteps
	for (int timestep = 0; timestep <= 6; timestep++){
		fprintf(stderr, "prod rank %d timestep %d\n", rank, timestep);
		for(int i = 0; i<3; ++i){
			array[i] = (i+1)*timestep*(rank+1)*pi;
		}

		SimpleFieldi d_index(rank);
		ArrayFieldf d_velocity(array,3, 1);

		pConstructData container0, container1;

		// Do the filtering manually
		container0->appendData("index", d_index,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		container0->appendData("velocity", d_velocity,
		                       DECAF_NOFLAG, DECAF_PRIVATE,
		                       DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		container1->appendData("velocity", d_velocity,
		                       DECAF_NOFLAG, DECAF_PRIVATE,
		                       DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		// sends the data to the output links
		ok = ok && decaf->put(container0, 0);
		ok = ok && decaf->put(container1, 1);
		if(!ok)
			break;
		usleep(200000);
	}
	decaf->terminate();
}

// prod2
void prod2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	float den = 2.71828;
	float den_array[3];
	bool ok = true;

	for (int timestep = 0; timestep <= 6; timestep++){
		fprintf(stderr, "prod2 rank %d timestep %d\n", rank, timestep);

		for(int i = 0; i<3; ++i){
			den_array[i] = (rank+1)*2*timestep*den*(i+1);
		}

		ArrayFieldf d_density(den_array, 3, 1);
		SimpleFieldi d_id(rank);

		pConstructData container2, container3;

		// Do the filtering manually
		container3->appendData("id", d_id,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

		container2->appendData("density", d_density,
		                      DECAF_NOFLAG, DECAF_PRIVATE,
		                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);


		// sends the data to the output links
		ok = ok && decaf->put(container2, 2);
		ok = ok && decaf->put(container3, 3);
		if(!ok)
			break;
		usleep(200000);
	}
	decaf->terminate();
}


// consumer
void con(Decaf* decaf)
{
	int rank = decaf->world->rank();
	map<int, pConstructData> in_data;

	while (decaf->get(in_data))
	{
		SimpleFieldi index;
		ArrayFieldf a_density, a_velocity;

		string s = "";
		index = in_data.at(0)->getFieldData<SimpleFieldi>("index");
		if(index){
			s+= "index ";
		}
		a_velocity = in_data.at(0)->getFieldData<ArrayFieldf>("velocity");
		if(a_velocity){
			s+= "velocity ";
		}
		a_density = in_data.at(2)->getFieldData<ArrayFieldf>("density");
		if(a_density){
			s+= "density ";
		}

		fprintf(stderr, "con of rank %d received: %s\n", rank, s.c_str());
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	//fprintf(stderr, "con1 %d terminating\n", rank);
	decaf->terminate();
}

// con2
void con2(Decaf* decaf)
{
	int rank = decaf->world->rank();
	map<int, pConstructData> in_data;

	while (decaf->get(in_data))
	{
		SimpleFieldi id;
		ArrayFieldf a_velocity;

		string s = "";
		id = in_data.at(3)->getFieldData<SimpleFieldi >("id");
		if(id){
			s+= "id ";
		}
		a_velocity = in_data.at(1)->getFieldData<ArrayFieldf>("velocity");
		if(a_velocity){
			    s+= "velocity ";
		}

		fprintf(stderr, "con2 of rank %d received: %s\n", rank, s.c_str());
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
	Workflow::make_wflow_from_json(workflow, "putget_index.json");

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

	return 0;
}
