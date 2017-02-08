//---------------------------------------------------------------------------
//
// 2 nodes example for testing user created object
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
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

using namespace decaf;
using namespace std;

// Simple class used for the test
class My_class{
public:
	My_class(float v = 0) : value(v){}
	~My_class(){}

	// Serialize method that must be implemented
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version){
		ar & value;
	}

	float value;
};

My_class operator+(My_class a, My_class b){
	return My_class(a.value + b.value);
}


// A VectorField<My_class> is used in the example so this registration is mandatory
BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<My_class>,"VectorConstructData<My_class>")

// producer
void prod(Decaf* decaf)
{
	int rank = decaf->world->rank();
	float pi = 3.1415;
	std::vector<My_class> v_object(3);
	// produce data for some number of timesteps
	for (int timestep = 1; timestep <3; timestep++){

		for(int i = 0; i<3; ++i){
			v_object[i].value = (i+1)*pi+rank*100;
		}

		VectorField<My_class> vf_object(v_object, 1);
		pConstructData container;

		container->appendData("object", vf_object,
		                      DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

		// send the data on all outbound dataflows, the filtering of contracts is done internaly
		if(! decaf->put(container) ){
			break;
		}
		fprintf(stdout, "prod rank %d timestep %d sent the object\n", rank, timestep);
		usleep(100000);
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
	decaf->terminate();
}

// consumer
void con(Decaf* decaf)
{
	int rank = decaf->world->rank();
	vector< pConstructData > in_data;
	std::vector<My_class> v_object;

	while (decaf->get(in_data))
	{
		// retrieve the values from the get
		for (size_t i = 0; i < in_data.size(); i++)
		{
			if(in_data[i]->hasData("object")){
				v_object = in_data[i]->getFieldData<VectorField<My_class>>("object").getVector();
			}
		}

		string s;
		for(size_t i = 0; i<v_object.size(); ++i){
			s+= std::to_string(v_object[i].value) + " ";
		}
		fprintf(stdout, "con rank %d received the values: %s\n", rank, s.c_str());
	}

	// terminate the task (mandatory) by sending a quit message to the rest of the workflow
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
		//fprintf(stdout, "Forwarding data in dflow, having %d fields\n", in_data->getNbFields());
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
	Workflow::make_wflow_from_json(workflow, "test_object.json");

	// run decaf
	run(workflow);

	return 0;
}
