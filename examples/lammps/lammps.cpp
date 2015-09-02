//---------------------------------------------------------------------------
//
// lammps example with new data model API
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

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <utility>
#include <map>

// lammps includes
#include "lammps.h"
#include "input.h"
#include "atom.h"
#include "library.h"

using namespace decaf;
using namespace LAMMPS_NS;
using namespace std;

struct lammps_args_t                         // custom args for running lammps
{
    LAMMPS* lammps;
    string infile;
};

struct pos_args_t                            // custom args for atom positions
{
    int natoms;                              // number of atoms
    double* pos;                             // atom positions
};

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
    // runs lammps and puts the atom positions to the dataflow at the consumer intervals
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void lammps(void* args,                       // arguments to the callback
                int t_current,                    // current time step
                int t_interval,                   // consumer time interval
                int t_nsteps,                     // total number of time steps
                vector<Dataflow*>* in_dataflows,  // all inbound dataflows
                vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "lammps redist\n");
        struct lammps_args_t* a = (lammps_args_t*)args; // custom args
        double* x;                           // atom positions

        if (t_current == 0)                  // first time step
        {
            // only create lammps for first dataflow instance
            a->lammps = new LAMMPS(0, NULL, (*out_dataflows)[0]->prod_comm_handle());
            a->lammps->input->file(a->infile.c_str());
        }

        a->lammps->input->one("run 1");
        int natoms = static_cast<int>(a->lammps->atom->natoms);
        x = new double[3 * natoms];
        lammps_gather_atoms(a->lammps, (char*)"x", 1, 3, x);

        if (!((t_current + 1) % t_interval))
        {
            for (size_t i = 0; i < out_dataflows->size(); i++)
            {
                shared_ptr<ConstructData> container = make_shared<ConstructData>();

                // lammps gathered all positions to rank 0
                if ((*out_dataflows)[i]->prod_comm()->rank() == 0)
                {
                    fprintf(stderr, "+ lammps redist producing time step %d with %d atoms\n",
                            t_current, natoms);
                    // debug
                    //         for (int i = 0; i < 10; i++)         // print first few atoms
                    //           fprintf(stderr, "%.3lf %.3lf %.3lf\n",
                    // x[3 * i], x[3 * i + 1], x[3 * i + 2]);

                    shared_ptr<VectorConstructData<double> > data  =
                            make_shared<VectorConstructData<double> >(x, 3 * natoms, 3);
                    container->appendData(string("pos"), data,
                                         DECAF_NOFLAG, DECAF_PRIVATE,
                                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
                }
                else
                {
                    vector<double> pos;
                    shared_ptr<VectorConstructData<double> > data  =
                            make_shared<VectorConstructData<double> >(pos, 3);
                    container->appendData(string("pos"), data,
                                         DECAF_NOFLAG, DECAF_PRIVATE,
                                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
                }
                (*out_dataflows)[i]->put(container, DECAF_PROD);
                (*out_dataflows)[i]->flush();       // need to clean up after each time step
            }
        }
        delete[] x;

        if (t_current == t_nsteps - 1)          // last time step
            delete a->lammps;
    }

    // gets the atom positions and prints them
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void print(void* args,                       // arguments to the callback
               int t_current,                    // current time step
               int t_interval,                   // consumer time interval
               int t_nsteps,                     // total number of time steps
               vector<Dataflow*>* in_dataflows,  // all inbound dataflows
               vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "print redist\n");
        if (!((t_current + 1) % t_interval))
        {
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            (*in_dataflows)[0]->get(container, DECAF_CON);

            shared_ptr<VectorConstructData<double> > pos =
                dynamic_pointer_cast<VectorConstructData<double> >
                (container->getData(string("pos")));

            // debug
            fprintf(stderr, "consumer redist print1 or print3 printing %d atoms\n",
                    pos->getNbItems());

            // debug
            //     for (int i = 0; i < 10; i++)               // print first few atoms
            //       fprintf(stderr, "%.3lf %.3lf %.3lf\n",
            // pos[3 * i], pos[3 * i + 1], pos[3 * i + 2]);

            (*in_dataflows)[0]->flush();            // need to clean up after each time step
        }
    }

    // puts the atom positions to the dataflow
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void print2(void* args,                       // arguments to the callback
                int t_current,                    // current time step
                int t_interval,                   // consumer time interval
                int t_nsteps,                     // total number of time steps
                vector<Dataflow*>* in_dataflows,  // all inbound dataflows
                vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        fprintf(stderr, "print2\n");

        if (!((t_current + 1) % t_interval))
        {
            // get
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            (*in_dataflows)[0]->get(container, DECAF_CON);

            (*in_dataflows)[0]->flush();        // need to clean up after each time step

            // put
            fprintf(stderr, "+ print2 forwarding time step %d\n", t_current);
            (*out_dataflows)[0]->put(container, DECAF_PROD);
            (*out_dataflows)[0]->flush();        // need to clean up after each time step
        }
    }

    // dataflow just needs to flush on every time step
    void dflow(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               Dataflow* dataflow)           // all dataflows
    {
        if(dataflow->is_dflow())             // TODO: will this ever be false?
        {
            // getting the data from the producer
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            dataflow->get(container, DECAF_DFLOW);
            shared_ptr<VectorConstructData<double> > pos =
                dynamic_pointer_cast<VectorConstructData<double> >
                (container->getData(string("pos")));

            fprintf(stderr, "- dataflowing redist time step %d, nbAtoms = %d\n",
                    t_current, pos->getNbItems());

            // forwarding the data to the consumers
            dataflow->put(container, DECAF_DFLOW);
            dataflow->flush();        // need to clean up after each time step
        }
    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int prod_nsteps,                    // number of producer time steps
         int con_nsteps,                     // number of consumer time steps
         string infile)                      // lammps input config file
{
    // callback args
    lammps_args_t lammps_args;               // custom args for lammps
    lammps_args.infile = infile;
    pos_args_t pos_args;                     // custom args for atom positions
    pos_args.pos = NULL;
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].func == "lammps")
            workflow.nodes[i].args = &lammps_args;
        if (workflow.nodes[i].func == "print2" || workflow.nodes[i].func  == "print")
            workflow.nodes[i].args = &pos_args;
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
    path.append(string("/examples/lammps/libmod_lammps.so"));
    string infile = argv[1];


    // fill workflow nodes
    WorkflowNode node;
    node.in_links.push_back(1);              // print1
    node.start_proc = 5;
    node.nprocs = 1;
    node.func = "print";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.in_links.push_back(0);              // print3
    node.start_proc = 9;
    node.nprocs = 1;
    node.func = "print";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.out_links.push_back(0);             // print2
    node.in_links.push_back(2);
    node.start_proc = 7;
    node.nprocs = 1;
    node.func = "print2";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.out_links.push_back(1);             // lammps
    node.out_links.push_back(2);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "lammps";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 2;                           // print2 - print3
    link.con = 1;
    link.start_proc = 8;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                           // lammps - print1
    link.con = 0;
    link.start_proc = 4;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                           // lammps - print2
    link.con = 2;
    link.start_proc = 6;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps, infile);

    return 0;
}
