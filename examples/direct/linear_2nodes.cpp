//---------------------------------------------------------------------------
//
// Direct coupling example with the new data model API
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
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void prod(void* args,                       // arguments to the callback
              int t_current,                    // current time step
              int t_interval,                   // consumer time interval
              int t_nsteps,                     // total number of time steps
              vector<Dataflow*>* in_dataflows,  // all inbound dataflows
              vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "prod\n");

        int* pd = (int*)args;                 // producer data
        *pd = t_current;                      // just assign something, current time step eg

        shared_ptr<SimpleConstructData<int> > data  =
            make_shared<SimpleConstructData<int> >( *pd );

        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        container->appendData(string("t_current"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // production time step
        if (!((t_current + 1) % t_interval))
        {
            fprintf(stderr, "+ producing time step %d\n", t_current);
            (*out_dataflows)[0]->put(container, DECAF_PROD);
            fprintf(stderr, "prod put done\n");
        }

        // final time step, send a terminate flag (user-defined)
        if (t_current == t_nsteps - 1)
        {
            *pd = -1;                          // assume -1 is not a valid value
            shared_ptr<SimpleConstructData<int> > data  =
                make_shared<SimpleConstructData<int> >( *pd );
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            container->appendData(string("t_current"), data,
                                  DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
            fprintf(stderr, "+ producing -1 at time step %d\n", t_current);
            (*out_dataflows)[0]->put(container, DECAF_PROD);
            fprintf(stderr, "prod put done\n");
        }
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void con(void* args,                       // arguments to the callback
             int t_current,                    // current time step
             int t_interval,                   // consumer time interval
             int t_nsteps,                     // total number of time steps
             vector<Dataflow*>* in_dataflows,  // all inbound dataflows
             vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "con\n");
        while (1)
        {
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            (*in_dataflows)[0]->get(container, DECAF_CON);
            fprintf(stderr, "con get done\n");
            shared_ptr<SimpleConstructData<int> > sum =
                dynamic_pointer_cast<SimpleConstructData<int> >
                (container->getData(string("t_current")));
            if (sum->getData() > 0)
                fprintf(stderr, "- consuming time step %d, sum = %d\n", t_current, sum->getData());
            else
            {
                fprintf(stderr, "con time step %d ending, sum = %d\n", t_current, sum->getData());
                break;
            }
        }
        fprintf(stderr, "con all done\n");
    }

    // dataflow just needs to flush on every time step
    void dflow(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               Dataflow* dataflow)           // dataflow
    {
        fprintf(stderr, "dflow\n");

        while(1)
        {
            // getting the data from the producer
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            dataflow->get(container, DECAF_DFLOW);
            fprintf(stderr, "dflow get done\n");
            shared_ptr<SimpleConstructData<int> > sum =
                dynamic_pointer_cast<SimpleConstructData<int> >
                (container->getData(string("t_current")));

            // fprintf(stderr, "- dataflowing time step %d, sum = %d\n", t_current, sum->getData());

            // forwarding the data to the consumers
            dataflow->put(container, DECAF_DFLOW);
            fprintf(stderr, "dflow put done\n");

            // check for termination
            if (sum->getData() < 0)
            {
                // fprintf(stderr, "dflow time step %d ending, sum = %d\n", t_current, sum->getData());
                break;
            }
        }
        // fprintf(stderr, "dflow all done\n");
    }
} // extern "C"

void run(Workflow& workflow,             // workflow
         int prod_nsteps,                // number of producer time steps
         int con_nsteps)                 // number of consumer time steps
{
    // callback args
    int *pd, *cd;
    pd = new int[1];
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].func == "prod")
            workflow.nodes[i].args = pd;
        if (workflow.nodes[i].func == "con")
            workflow.nodes[i].args = cd;
    }

    MPI_Init(NULL, NULL);

    // create and run decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow, prod_nsteps, con_nsteps);
    Data data(MPI_INT);
    decaf->run(&data, &pipeliner, &checker);

    // cleanup
    delete[] pd;
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// this is hard-coding the no overlap case
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    Workflow workflow;
    int prod_nsteps = 4;
    int con_nsteps = 2;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. "
                "Please export DECAF_PREFIX to point to the root of your decaf "
                "install directory.\n");
        exit(1);
    }

    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/libmod_linear_2nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                        // producer
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "prod";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // consumer
    node.in_links.clear();
    node.in_links.push_back(0);
    node.start_proc = 6;
    node.nprocs = 2;
    node.func = "con";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow link
    WorkflowLink link;
    link.prod = 0;                               // dataflow
    link.con = 1;
    link.start_proc = 4;
    link.nprocs = 2;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
