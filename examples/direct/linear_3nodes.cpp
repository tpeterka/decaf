//---------------------------------------------------------------------------
//
// 3-node linear coupling example
//
// node0 (4 procs) - node1 (2 procs) - node2 (1 proc)
// node0[0,3] -> dflow[4,6] -> node1[7,8] -> dflow[9,10] -> node2[11]
//
// entire workflow takes 12 procs (3 dataflow procs between node0 and node1 and
// 2 dataflow procs between node1 and node2)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>
#include <decaf/data_model/arrayconstructdata.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

using namespace decaf;
using namespace std;

// user-defined pipeliner code
void pipeliner(Dataflow* decaf)
{
}

// user-defined resilience code
void checker(Dataflow* decaf)
{
}

// node and link callback functions
extern "C"
{
    // producer
    int node0(void* args,                                   // arguments to the callback
              vector<Dataflow*>* out_dataflows,             // all outbound dataflows
              vector< shared_ptr<ConstructData> >* in_data) // input data in order of
    {
        static int timestep = 0;

        // produce data for some number of timesteps
        if (timestep < 10)
        {
            fprintf(stderr, "producer timestep %d\n", timestep);

            // the data in this example is just the timestep; put it in a container
            shared_ptr<SimpleConstructData<int> > data =
                make_shared<SimpleConstructData<int> >(timestep);
            shared_ptr<ConstructData> container = make_shared<ConstructData>();

            // send the data to the destinations
            container->appendData(string("var"), data,
                                  DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
            for (size_t i = 0; i < out_dataflows->size(); i++)
                (*out_dataflows)[i]->put(container, DECAF_PROD);

            timestep++;
            return 0;                        // ok to call me again
        }
        else
        {
            // send a quit message
            fprintf(stderr, "producer sending quit\n");
            shared_ptr<ConstructData> quit_container = make_shared<ConstructData>();
            Dataflow::set_quit(quit_container);
            for (size_t i = 0; i < out_dataflows->size(); i++)
                (*out_dataflows)[i]->put(quit_container, DECAF_PROD);

            return 1;                            // I quit, don't call me anymore
        }
    }

    // intermediate node is both a consumer and producer
    int node1(void* args,                                   // arguments to the callback
              vector<Dataflow*>* out_dataflows,             // all outbound dataflows
              vector< shared_ptr<ConstructData> >* in_data) // input data in order of
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data->size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = (*in_data)[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
                fprintf(stderr, "node1: sum = %d\n", sum);
            }
        }

        shared_ptr<SimpleConstructData<int> > data  = make_shared<SimpleConstructData<int> >(sum);
        shared_ptr<ConstructData> container = make_shared<ConstructData>();

        // send the sum
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
        for (size_t i = 0; i < out_dataflows->size(); i++)
            (*out_dataflows)[i]->put(container, DECAF_PROD);

        return 0;                            // ok to call me again
    }

    // consumer
    int node2(void* args,                                   // arguments to the callback
              vector<Dataflow*>* out_dataflows,             // all outbound dataflows
              vector< shared_ptr<ConstructData> >* in_data) // input data in order of
    {
        int sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data->size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = (*in_data)[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
            }
        }
        fprintf(stderr, "consumer sum = %d\n", sum);
        return 0;                            // ok to call me again
    }

    // dataflow just forwards everything that comes its way in this example
    int dflow(void* args,                          // arguments to the callback
              Dataflow* dataflow,                  // dataflow
              shared_ptr<ConstructData> in_data)   // input data
    {
        dataflow->put(in_data, DECAF_DFLOW);
        return 0;                            // ok to call me again
    }
} // extern "C"

void run(Workflow&          workflow,                // workflow
         // const vector<int>& sources = vector<int>()) // optional source workflow nodes
         const vector<int>& sources)                 // optional source workflow nodes
{
    // optional callback args, can leave uninitialized if not being used
    for (size_t i = 0; i < workflow.nodes.size(); i++)
        workflow.nodes[i].args = NULL;              // define any callback args here

    MPI_Init(NULL, NULL);

    // create and run decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);
    decaf->run(&pipeliner, &checker, sources);

    MPI_Finalize();
}

// test driver for debugging purposes
// this is hard-coding the no overlap case
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    Workflow workflow;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }

    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/mod_linear_3nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                        // node0
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "node0";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // node1
    node.in_links.clear();
    node.in_links.push_back(0);
    node.out_links.push_back(1);
    node.start_proc = 7;
    node.nprocs = 2;
    node.func = "node1";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // node2
    node.in_links.clear();
    node.in_links.push_back(1);
    node.start_proc = 11;
    node.nprocs = 1;
    node.func = "node2";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 0;                               // dataflow between node0 and node1
    link.con = 1;
    link.start_proc = 4;
    link.nprocs = 3;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 1;                               // dataflow between node1 and node2
    link.con = 2;
    link.start_proc = 9;
    link.nprocs = 2;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow);

    return 0;
}
