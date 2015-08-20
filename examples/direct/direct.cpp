//---------------------------------------------------------------------------
//
// example of direct coupling
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>

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
    void prod(void* args,                   // arguments to the callback
              int t_current,                // current time step
              int t_interval,               // consumer time interval
              int t_nsteps,                 // total number of time steps
              vector<Dataflow*>* dataflows, // all dataflows (for producer or consumer)
              int this_dataflow = -1)       // index of one dataflow in list of all

    {
        fprintf(stderr, "prod\n");

        int* pd = (int*)args;               // producer data
        *pd = t_current;                    // just assign something, current time step for example

        if (!((t_current + 1) % t_interval))
        {
            for (size_t i = 0; i < dataflows->size(); i++)
            {
                fprintf(stderr, "+ producing time step %d\n", t_current);
                (*dataflows)[i]->put(pd);
                (*dataflows)[i]->flush();           // need to clean up after each time step
            }
        }
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void con(void* args,                     // arguments to the callback
             int t_current,                  // current time step
             int t_interval,                 // consumer time interval
             int t_nsteps,                   // total number of time steps
             vector<Dataflow*>* dataflows,   // all dataflows (for producer or consumer)
             int this_dataflow = -1)         // index of one dataflow in list of all
    {
        if (!((t_current + 1) % t_interval))
        {
            int* cd = (int*)(*dataflows)[0]->get(); // we know dataflow.size() = 1 in this example

            // for example, add all the items arrived at this rank
            int sum = 0;
            for (int i = 0; i < (*dataflows)[0]->get_nitems(); i++)
                sum += cd[i];
            fprintf(stderr, "- consuming time step %d, sum = %d\n", t_current, sum);

            (*dataflows)[0]->flush();        // need to clean up after each time step
        }
    }

    // dataflow just needs to flush on every time step
    void dflow(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               vector<Dataflow*>* dataflows, // all dataflows
               int this_dataflow = -1)       // index of one dataflow in list of all
    {
        fprintf(stderr, "dflow\n");
        for (size_t i = 0; i < dataflows->size(); i++)
        {
            //Copy from the run function of Dataflow
            if ((*dataflows)[i]->is_dflow() && !(*dataflows)[i]->is_prod())
            {
                (*dataflows)[i]->forward();
            }
            (*dataflows)[i]->flush();        // need to clean up after each time step
        }
    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int prod_nsteps,                    // number of producer time steps
         int con_nsteps)                     // number of consumer time steps
{
    // callback args
    int *pd, *cd;
    pd = new int[1];
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].prod_func == "prod")
            workflow.nodes[i].prod_args = pd;
        if (workflow.nodes[i].prod_func == "con")
            workflow.nodes[i].prod_args = cd;
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
                <<" Please export DECAF_PREFIX to point to the root of you decaf install directory."<<std::endl;
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/libmod_direct.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                  // producer
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;
    node.prod_func = "prod";
    node.con_func = "";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                  // consumer
    node.in_links.clear();
    node.in_links.push_back(0);
    // node.start_proc = 6;                  // no overlap
    node.start_proc = 2;                     // partial overlap
    // node.start_proc = 0;                  // total overlap
    node.nprocs = 2;                         // no or partial overlap
    // node.nprocs = 4;                      // total overlap
    node.prod_func = "";
    node.con_func = "con";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow link
    WorkflowLink link;
    link.prod = 0;                           // dataflow
    link.con = 1;
    // link.start_proc = 4;                  // no overlap
    link.start_proc = 1;                     // partial overlap
    // link.start_proc = 0;                  // total overlap
    link.nprocs = 2;                         // no or partial overlap
    // link.nprocs = 4;                      // total overlap
    link.dflow_func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
