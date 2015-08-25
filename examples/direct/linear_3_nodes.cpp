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
    void node0(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               vector<Dataflow*>* dataflows, // all dataflows (for producer or consumer)
               int this_dataflow = -1)       // index of one dataflow in list of all

    {
        fprintf(stderr, "node0: prod\n");

        int* pd = (int*)args;                 // producer data
        *pd = t_current;                      // just assign something, current time step for example

//         std::shared_ptr<SimpleConstructData<int> > data  =
//             std::make_shared<SimpleConstructData<int> >( *pd );

//         std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
//         container->appendData(std::string("t_current"), data,
//                               DECAF_NOFLAG, DECAF_PRIVATE,
//                               DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

//         if (!((t_current + 1) % t_interval))
//         {
//             for (size_t i = 0; i < dataflows->size(); i++)
//             {
//                 fprintf(stderr, "+ producing time step %d\n", t_current);
//                 (*dataflows)[i]->put(container, DECAF_PROD);
//                 (*dataflows)[i]->flush();
//                 std::cout<<"Prod Put done"<<std::endl;
//             }
//         }
    }

    // intermediate node
    void node1(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               vector<Dataflow*>* dataflows, // all dataflows
               int this_dataflow = -1)       // index of one dataflow in list of all
    {
//         fprintf(stderr, "node1: both\n");

//         // get
//         shared_ptr<ConstructData> get_container = make_shared<ConstructData>();
//         (*dataflows)[0]->get(get_container, DECAF_CON);
//         cout << "node1: get done" << endl;
//         shared_ptr<SimpleConstructData<int> > sum =
//             dynamic_pointer_cast<SimpleConstructData<int> >
//             (get_container->getData(string("t_current")));

//         // put
//         shared_ptr<SimpleConstructData<int> > data  =
//             make_shared<SimpleConstructData<int> >(sum->getData());
//         shared_ptr<ConstructData> put_container = make_shared<ConstructData>();
//         put_container->appendData(string("node1"), data,
//                               DECAF_NOFLAG, DECAF_PRIVATE,
//                               DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
//         if (!((t_current + 1) % t_interval))
//         {
//             for (size_t i = 0; i < dataflows->size(); i++)
//             {
//                 (*dataflows)[i]->put(put_container, DECAF_PROD);
//                 (*dataflows)[i]->flush();
//                 cout << "node1: put done" << endl;
//             }
//         }
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void node2(void* args,                     // arguments to the callback
               int t_current,                  // current time step
               int t_interval,                 // consumer time interval
               int t_nsteps,                   // total number of time steps
               vector<Dataflow*>* dataflows,   // all dataflows (for producer or consumer)
               int this_dataflow = -1)         // index of one dataflow in list of all
    {
//         if (!((t_current + 1) % t_interval))
//         {
//             std::cout<<"node2: con"<<std::endl;
//             std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
//             (*dataflows)[0]->get(container, DECAF_CON);
//             std::cout<<"Get done"<<std::endl;
//             std::shared_ptr<SimpleConstructData<int> > sum =
//                 dynamic_pointer_cast<SimpleConstructData<int> >
//                 (container->getData(std::string("node1")));

//             // for this example, the policy of the redistribute component is add
//             fprintf(stderr, "- consuming time step %d, sum = %d\n", t_current, sum->getData());
//         }
    }

    // dataflow just needs to flush on every time step
    void dflow(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               vector<Dataflow*>* dataflows, // all dataflows
               int this_dataflow = -1)       // index of one dataflow in list of all
    {
//         fprintf(stderr, "dflow\n");
//         for (size_t i = 0; i < dataflows->size(); i++)
//         {
//             //Getting the data from the producer
//             std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
//             (*dataflows)[i]->get(container, DECAF_DFLOW);
//             std::cout<<"dflow get done"<<std::endl;
//             std::shared_ptr<SimpleConstructData<int> > sum =
//                 dynamic_pointer_cast<SimpleConstructData<int> >
//                 (container->getData(std::string("t_current")));

//             fprintf(stderr, "- dataflowing time step %d, sum = %d\n", t_current, sum->getData());

//             // forwarding the data to the consumers
//             (*dataflows)[i]->put(container, DECAF_DFLOW);
//             (*dataflows)[i]->flush();
//             std::cout<<"dflow put done"<<std::endl;
//         }
    }
} // extern "C"

void run(Workflow& workflow,             // workflow
         int prod_nsteps,                // number of producer time steps
         int con_nsteps)                 // number of consumer time steps
{
    // callback args
    int *pd, *cd, *bd;                       // args for producer, consumer, both
    pd = new int[1];
    // TODO: allocate, initialize bd?
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].type == DECAF_PROD)
            workflow.nodes[i].args = pd;
        else if (workflow.nodes[i].type == DECAF_CON)
            workflow.nodes[i].args = cd;
        else if (workflow.nodes[i].type == DECAF_BOTH)
            workflow.nodes[i].args = bd;
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
    int con_nsteps = 4;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        std::cout<<"ERROR : environment variable DECAF_PREFIX not defined."
                 <<" Please export DECAF_PREFIX to point to the root of your decaf install directory."<<std::endl;
        exit(1);
    }

    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/libmod_linear_3_nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                        // node0
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;

    // TODO: DEPECATED, remove
    node.prod_func = "";
    node.con_func = "";

    node.func = "node0";
    node.type = DECAF_PROD;
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // node1
    node.in_links.clear();
    node.in_links.push_back(0);
    node.out_links.push_back(1);
    node.start_proc = 6;
    node.nprocs = 3;

    // TODO: DEPECATED, remove
    node.prod_func = "";
    node.con_func = "";

    node.func = "node1";
    node.type = DECAF_BOTH;
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // node2
    node.in_links.clear();
    node.in_links.push_back(1);
    node.start_proc = 10;
    node.nprocs = 2;

    // TODO: DEPECATED, remove
    node.prod_func = "";
    node.con_func = "";

    node.func = "node2";
    node.type = DECAF_CON;
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 0;                               // dataflow between node1 and node2
    link.con = 1;
    link.start_proc = 4;
    link.nprocs = 2;

    // TODO: DEPECATED, remove
    link.dflow_func = "";

    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 1;                               // dataflow between node2 and node3
    link.con = 2;
    link.start_proc = 9;
    link.nprocs = 1;

    // TODO: DEPECATED, remove
    link.dflow_func = "";

    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
