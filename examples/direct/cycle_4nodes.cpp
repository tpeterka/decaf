//---------------------------------------------------------------------------
//
// 4-node graph with 2 cycles
//
//      __  node_b (1 proc)
//     /   /
//    node_a (4 procs)
//         \
//          node_c (1 proc) - node_d (1 proc)
//             \ ______________ /
//
//  entire workflow takes 12 procs (1 dataflow proc between each producer consumer pair)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>
#include <decaf/data_model/constructtype.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <utility>
#include <map>

using namespace decaf;
using namespace std;

// user-defined pipeliner code
void pipeliner(Dataflow* dataflow)
{
}

// user-defined resilience code
void checker(Dataflow* dataflow)
{
}

// node and link callback functions
extern "C"
{
    void node_a(void* args,                       // arguments to the callback
                int t_current,                    // current time step
                int t_interval,                   // consumer time interval
                int t_nsteps,                     // total number of time steps
                vector<Dataflow*>* in_dataflows,  // all inbound dataflows
                vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "node_a\n");

        if (t_current == 0)                  // first time step
        {
        }

        if (!((t_current + 1) % t_interval)) // analyze this time step
        {
        }

        if (t_current == t_nsteps - 1)          // last time step
            ;
    }

    void node_b(void* args,                       // arguments to the callback
               int t_current,                    // current time step
               int t_interval,                   // consumer time interval
               int t_nsteps,                     // total number of time steps
               vector<Dataflow*>* in_dataflows,  // all inbound dataflows
               vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "node_b\n");

        if (!((t_current + 1) % t_interval))
        {
        }
    }

    void node_c(void* args,                       // arguments to the callback
                int t_current,                    // current time step
                int t_interval,                   // consumer time interval
                int t_nsteps,                     // total number of time steps
                vector<Dataflow*>* in_dataflows,  // all inbound dataflows
                vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "node_c\n");

        if (!((t_current + 1) % t_interval))
        {
        }
    }

    void node_d(void* args,                       // arguments to the callback
                int t_current,                    // current time step
                int t_interval,                   // consumer time interval
                int t_nsteps,                     // total number of time steps
                vector<Dataflow*>* in_dataflows,  // all inbound dataflows
                vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "node_d\n");

        if (!((t_current + 1) % t_interval))
        {
        }
    }

    // dataflow just needs to flush on every time step
    void dflow(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               Dataflow* dataflow)           // dataflow
    {
        fprintf(stderr, "dflow\n");

    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int prod_nsteps,                    // number of producer time steps
         int con_nsteps)                     // number of consumer time steps
{
    // callback args
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].func == "node_a")
            ;                                // TODO: set up args
        if (workflow.nodes[i].func == "node_b")
            ;                                // TODO: set up args
        if (workflow.nodes[i].func == "node_c")
            ;                                // TODO: set up args
        if (workflow.nodes[i].func == "node_d")
            ;                                // TODO: set up args
    }

    MPI_Init(NULL, NULL);

    // create and run decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow, prod_nsteps, con_nsteps);
    Data data(MPI_DOUBLE);
    decaf->run(&data, &pipeliner, &checker);

    MPI_Barrier(MPI_COMM_WORLD);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    Workflow workflow;
    int prod_nsteps = 1;
    int con_nsteps = 1;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        cout<<"ERROR : environment variable DECAF_PREFIX not defined."
                <<" Please export DECAF_PREFIX to point to the root of you decaf install directory."<<endl;
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/lammps/libmod_cycle_4nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.in_links.push_back(1);              // node_b
    node.start_proc = 5;
    node.nprocs = 1;
    node.func = "node_b";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.in_links.push_back(0);              // node_d
    node.start_proc = 9;
    node.nprocs = 1;
    node.func = "node_d";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.out_links.push_back(0);             // node_c
    node.in_links.push_back(2);
    node.start_proc = 7;
    node.nprocs = 1;
    node.func = "node_c";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.out_links.push_back(1);             // node_a
    node.out_links.push_back(2);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "node_a";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 2;                           // node_c -> node_d
    link.con = 1;
    link.start_proc = 8;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                           // node_a -> node_b
    link.con = 0;
    link.start_proc = 4;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                           // node_a -> node_c
    link.con = 2;
    link.start_proc = 6;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 1;                           // node_d -> node_c
    link.con = 2;
    link.start_proc = 10;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                           // node_b -> node_a
    link.con = 0;
    link.start_proc = 11;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
