//---------------------------------------------------------------------------
//
// lammps example
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>
#include "../include/ConstructType.hpp"

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
    void lammps(void* args,                  // arguments to the callback
                int t_current,               // current time step
                int t_interval,              // consumer time interval
                int t_nsteps,                // total number of time steps
                vector<Dataflow*>* dataflows,// all dataflows (for producer or consumer)
                int this_dataflow = -1)      // index of one dataflow in list of all

    {
        fprintf(stderr, "lammps redist\n");
        struct lammps_args_t* a = (lammps_args_t*)args; // custom args
        double* x;                           // atom positions

        if (t_current == 0)                  // first time step
        {
            // only create lammps for first dataflow instance
            a->lammps = new LAMMPS(0, NULL, (*dataflows)[0]->prod_comm_handle());
            a->lammps->input->file(a->infile.c_str());
        }

        a->lammps->input->one("run 1");
        int natoms = static_cast<int>(a->lammps->atom->natoms);
        x = new double[3 * natoms];
        lammps_gather_atoms(a->lammps, (char*)"x", 1, 3, x);

        if (!((t_current + 1) % t_interval))
        {
            for (size_t i = 0; i < dataflows->size(); i++)
            {
                std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
                if ((*dataflows)[i]->prod_comm()->rank() == 0) // lammps gathered all positions to rank 0
                {
                    fprintf(stderr, "+ lammps redist producing time step %d with %d atoms\n", t_current, natoms);
                    // debug
                    //         for (int i = 0; i < 10; i++)         // print first few atoms
                    //           fprintf(stderr, "%.3lf %.3lf %.3lf\n", x[3 * i], x[3 * i + 1], x[3 * i + 2]);
                    //(*dataflows)[i]->put(x, 3 * natoms);
                    std::shared_ptr<ArrayConstructData<double> > data  =
                            std::make_shared<ArrayConstructData<double> >(x, 3 * natoms, 3);
                    container->appendData(std::string("pos"), data,
                                         DECAF_NOFLAG, DECAF_PRIVATE,
                                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
                }
                else
                {
                    //(*dataflows)[i]->put(NULL); // put is collective; all producer ranks must call it
                    std::vector<double> pos;
                    std::shared_ptr<ArrayConstructData<double> > data  =
                            std::make_shared<ArrayConstructData<double> >(pos, 3);
                    container->appendData(std::string("pos"), data,
                                         DECAF_NOFLAG, DECAF_PRIVATE,
                                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
                }
                (*dataflows)[i]->put(container, DECAF_PROD);
                (*dataflows)[i]->flush();       // need to clean up after each time step
            }
        }
        delete[] x;

        if (t_current == t_nsteps - 1)          // last time step
            delete a->lammps;
    }

    // gets the atom positions and prints them
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void print(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               vector<Dataflow*>* dataflows, // all dataflows (for producer or consumer)
               int this_dataflow = -1)       // index of one dataflow in list of all
    {
        fprintf(stderr, "print redist\n");
        if (!((t_current + 1) % t_interval))
        {
            //double* pos    = (double*)(*dataflows)[0]->get(); // we know dataflow.size() = 1 in this example
            std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
            (*dataflows)[0]->get(container, DECAF_CON);

            std::shared_ptr<ArrayConstructData<double> > pos =
                    dynamic_pointer_cast<ArrayConstructData<double> >(container->getData(std::string("pos")));

            // debug
            fprintf(stderr, "consumer redist print1 or print3 printing %d atoms\n",
                    pos->getNbItems());
            //     for (int i = 0; i < 10; i++)               // print first few atoms
            //       fprintf(stderr, "%.3lf %.3lf %.3lf\n", pos[3 * i], pos[3 * i + 1], pos[3 * i + 2]);
            (*dataflows)[0]->flush();        // need to clean up after each time step
        }
    }

    // puts the atom positions to the dataflow
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void print2_prod(void* args,             // arguments to the callback
                     int t_current,          // current time step
                     int t_interval,         // consumer time interval
                     int t_nsteps,           // total number of time steps
                     vector<Dataflow*>* dataflows, // all dataflows (for producer or consumer)
                     int this_datafow = -1)  // index of one dataflow in list of all
    {
        fprintf(stderr, "print2_prod redist \n");
        struct pos_args_t* a = (pos_args_t*)args;   // custom args

        if (!((t_current + 1) % t_interval))
        {
            std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
            fprintf(stderr, "+ print2 redist producing time step %d with %d atoms\n", t_current, a->natoms);
            std::shared_ptr<ArrayConstructData<double> > data  =
                    std::make_shared<ArrayConstructData<double> >(a->pos, 3 * a->natoms, 3);
            container->appendData(std::string("pos"), data,
                                 DECAF_NOFLAG, DECAF_PRIVATE,
                                 DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
            //(*dataflows)[0]->put(a->pos, a->natoms * 3);
            (*dataflows)[0]->put(container, DECAF_PROD);
            (*dataflows)[0]->flush();        // need to clean up after each time step
            delete[] a->pos;
        }
    }

    // gets the atom positions and copies them
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void print2_con(void* args,              // arguments to the callback
                    int t_current,           // current time step
                    int t_interval,          // consumer time interval
                    int t_nsteps,            // total number of time steps
                    vector<Dataflow*>* dataflows, // all dataflows (for producer or consumer)
                    int this_dataflow = -1)  // index of one dataflow in list of all
    {
        fprintf(stderr, "print2_con redist\n");
        struct pos_args_t* a = (pos_args_t*)args;    // custom args
        if (!((t_current + 1) % t_interval))
        {
            // we know dataflows.size() = 1 in this example
            std::cout<<"Number of dataflows : "<<dataflows->size()<<std::endl;
            std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
            (*dataflows)[0]->get(container, DECAF_CON);

            std::shared_ptr<ArrayConstructData<double> > data =
                    dynamic_pointer_cast<ArrayConstructData<double> >(container->getData(std::string("pos")));
            std::vector<double>& pos = data->getArray();
            a->natoms   = data->getNbItems();
            a->pos      = new double[a->natoms * 3];
            fprintf(stderr, "consumer redist print 2 copying %d atoms\n", a->natoms);
            for (int i = 0; i < a->natoms; i++)
            {
                a->pos[3 * i    ] = pos[3 * i    ];
                a->pos[3 * i + 1] = pos[3 * i + 1];
                a->pos[3 * i + 2] = pos[3 * i + 2];
            }
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
        int rank_dflow;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank_dflow);

        for (size_t i = 0; i < dataflows->size(); i++)
        {
            if(((*dataflows)[i])->is_dflow())
            {
                //Getting the data from the producer
                std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
                (*dataflows)[i]->get(container, DECAF_DFLOW);
                std::shared_ptr<ArrayConstructData<double> > pos =
                        dynamic_pointer_cast<ArrayConstructData<double> >(container->getData(std::string("pos")));

                fprintf(stderr, "- dataflowing redist time step %d, nbAtoms = %d\n", t_current, pos->getNbItems());

                //Forwarding the data to the consumers
                (*dataflows)[i]->put(container, DECAF_DFLOW);
                (*dataflows)[i]->flush();        // need to clean up after each time step
            }
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
        if (workflow.nodes[i].prod_func == "lammps")
            workflow.nodes[i].prod_args = &lammps_args;
        if (workflow.nodes[i].prod_func == "print2_prod")
            workflow.nodes[i].prod_args = &pos_args;
        if (workflow.nodes[i].con_func  == "print2_con"|| workflow.nodes[i].con_func  == "print")
            workflow.nodes[i].con_args  = &pos_args;
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
        std::cout<<"ERROR : environment variable DECAF_PREFIX not defined."
                <<" Please export DECAF_PREFIX to point to the root of you decaf install directory."<<std::endl;
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/lammps/libmod_lammps_redist.so"));
    string infile = argv[1];


    // fill workflow nodes
    WorkflowNode node;
    node.in_links.push_back(1);              // print1
    node.start_proc = 5;
    node.nprocs = 1;
    node.prod_func = "";
    node.con_func = "print";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.in_links.push_back(0);              // print3
    node.start_proc = 9;
    node.nprocs = 1;
    node.prod_func = "";
    node.con_func = "print";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.out_links.push_back(0);             // print2
    node.in_links.push_back(2);
    node.start_proc = 7;
    node.nprocs = 1;
    node.prod_func = "print2_prod";
    node.con_func = "print2_con";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();
    node.in_links.clear();
    node.out_links.push_back(1);             // lammps
    node.out_links.push_back(2);
    node.start_proc = 0;
    node.nprocs = 4;
    node.prod_func = "lammps";
    node.con_func = "";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 2;                           // print2 - print3
    link.con = 1;
    link.start_proc = 8;
    link.nprocs = 1;
    link.dflow_func = "dflow";
    link.path = path;
    workflow.links.push_back(link);

    link.prod = 3;                           // lammps - print1
    link.con = 0;
    link.start_proc = 4;
    link.nprocs = 1;
    link.dflow_func = "dflow";
    link.path = path;
    workflow.links.push_back(link);

    link.prod = 3;                           // lammps - print2
    link.con = 2;
    link.start_proc = 6;
    link.nprocs = 1;
    link.dflow_func = "dflow";
    link.path = path;
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps, infile);

    return 0;
}
