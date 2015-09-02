//---------------------------------------------------------------------------
//
// 3-node linear coupling example with the new data model API
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
    void node0(void* args,                       // arguments to the callback
               int t_current,                    // current time step
               int t_interval,                   // consumer time interval
               int t_nsteps,                     // total number of time steps
               vector<Dataflow*>* in_dataflows,  // all inbound dataflows
               vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        // cerr << "node0: prod, out_dataflows.size = " << out_dataflows->size() << endl;
        int* pd = (int*)args;                 // producer data
        *pd = t_current;                      // just assign something, current time step for now
        shared_ptr<SimpleConstructData<int> > data  =
            make_shared<SimpleConstructData<int> >( *pd );
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        container->appendData(string("node0"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        // put data to the dataflow
        if (!((t_current + 1) % t_interval))
        {
            cerr << "+ producing time step " << t_current << endl;
            (*out_dataflows)[0]->put(container, DECAF_PROD);
            (*out_dataflows)[0]->flush();
            cerr<<"node0: put done"<<endl;
        }
    }

    // dataflow between nodes 0 and 1 just needs to flush on every time step
    void dflow01(void* args,                   // arguments to the callback
                 int t_current,                // current time step
                 int t_interval,               // consumer time interval
                 int t_nsteps,                 // total number of time steps
                 Dataflow* dataflow)           // dataflow
    {
        // getting the data from the producer
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        dataflow->get(container, DECAF_DFLOW);
        // cerr<<"dflow01: get done"<<endl;
        shared_ptr<SimpleConstructData<int> > sum =
            dynamic_pointer_cast<SimpleConstructData<int> >
            (container->getData(string("node0")));

        cerr << "- dataflowing 0->1 time " << t_current << " sum " << sum->getData() << endl;

        // forward the data to the consumers
        dataflow->put(container, DECAF_DFLOW);
        dataflow->flush();
        cerr<<"dflow01: put done"<<endl;
    }

    // intermediate node
    void node1(void* args,                       // arguments to the callback
               int t_current,                    // current time step
               int t_interval,                   // consumer time interval
               int t_nsteps,                     // total number of time steps
               vector<Dataflow*>* in_dataflows,  // all inbound dataflows
               vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        // get
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        (*in_dataflows)[0]->get(container, DECAF_CON);
        cerr << "node1: get done" << endl;

        (*in_dataflows)[0]->flush();        // need to clean up after each time step

        // put
        if (!((t_current + 1) % t_interval))
        {
            (*out_dataflows)[0]->put(container, DECAF_PROD);
            (*out_dataflows)[0]->flush();
            cerr << "node1: put done" << endl;
        }
    }

    // dataflow between nodes 1 and 2 just needs to flush on every time step
    void dflow12(void* args,                   // arguments to the callback
                 int t_current,                // current time step
                 int t_interval,               // consumer time interval
                 int t_nsteps,                 // total number of time steps
                 Dataflow* dataflow)           // dataflow
    {
        cerr << "dflow12" << endl;
        // getting the data from the producer
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        dataflow->get(container, DECAF_DFLOW);
        cerr<<"dflow12: get done"<<endl;
        shared_ptr<SimpleConstructData<int> > sum =
            dynamic_pointer_cast<SimpleConstructData<int> >
            (container->getData(string("node0")));

        cerr << "- dataflowing 1->2 time " << t_current << " sum " << sum->getData() << endl;

        // forwarding the data to the consumers
        dataflow->put(container, DECAF_DFLOW);
        dataflow->flush();
        cerr<<"dflow12: put done"<<endl;
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void node2(void* args,                       // arguments to the callback
               int t_current,                    // current time step
               int t_interval,                   // consumer time interval
               int t_nsteps,                     // total number of time steps
               vector<Dataflow*>* in_dataflows,  // all inbound dataflows
               vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        if (!((t_current + 1) % t_interval))
        {
            cerr << "node2: con, in_dataflows.size = " << in_dataflows->size() << endl;
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            (*in_dataflows)[0]->get(container, DECAF_CON);
            cerr<<"node2: get done"<<endl;
            shared_ptr<SimpleConstructData<int> > sum =
                dynamic_pointer_cast<SimpleConstructData<int> >
                (container->getData(string("node0")));

            // for this example, the policy of the redistribute component is add
            cerr << "- consuming time step " << t_current << " sum = " << sum->getData() << endl;
        }
    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int prod_nsteps,                    // number of producer time steps
         int con_nsteps)                     // number of consumer time steps
{
    // callback args
    int *pd;                                 // args for producer, consumer, both
    pd = new int[1];
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (!workflow.nodes[i].in_links.size())
            workflow.nodes[i].args = pd;
        else
            workflow.nodes[i].args = NULL;
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
    int prod_nsteps = 2;
    int con_nsteps = 2;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        cerr<<"ERROR : environment variable DECAF_PREFIX not defined."
                 <<" Please export DECAF_PREFIX to point to the root of your decaf install directory."<<endl;
        exit(1);
    }

    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/libmod_linear_3nodes.so"));

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
    link.prod = 0;                               // dataflow between node1 and node2
    link.con = 1;
    link.start_proc = 4;
    link.nprocs = 3;
    link.func = "dflow01";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 1;                               // dataflow between node2 and node3
    link.con = 2;
    link.start_proc = 9;
    link.nprocs = 2;
    link.func = "dflow12";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
