//---------------------------------------------------------------------------
//
// lammps example
//
// 4-node workflow
//
//          print (1 proc)
//        /
//    lammps (4 procs)
//        \
//          print2 (1 proc) - print (1 proc)
//
//  entire workflow takes 10 procs (1 dataflow proc between each producer consumer pair)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/boost_macros.h>
#ifdef TRANSPORT_CCI
#include <cci.h>
#endif

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

// runs lammps and puts the atom positions to the dataflow at the consumer intervals
void lammps(Decaf* decaf, int nsteps, int analysis_interval, string infile)
{
    LAMMPS* lps = new LAMMPS(0, NULL, decaf->prod_comm_handle());
    lps->input->file(infile.c_str());

    for (int timestep = 0; timestep < nsteps; timestep++)
    {
        fprintf(stderr, "lammps\n");

        lps->input->one("run 1");
        int natoms = static_cast<int>(lps->atom->natoms);
        double* x = new double[3 * natoms];
        lammps_gather_atoms(lps, (char*)"x", 1, 3, x);

        if (!((timestep + 1) % analysis_interval))
        {
            pConstructData container;

            // lammps gathered all positions to rank 0
            if (decaf->prod_comm()->rank() == 0)
            {
                fprintf(stderr, "lammps producing time step %d with %d atoms\n",
                        timestep, natoms);
                // debug
                //         for (int i = 0; i < 10; i++)         // print first few atoms
                //           fprintf(stderr, "%.3lf %.3lf %.3lf\n",
                // x[3 * i], x[3 * i + 1], x[3 * i + 2]);

                VectorFliedd data(x, 3 * natoms, 3);

                container->appendData("pos", data,
                                      DECAF_NOFLAG, DECAF_PRIVATE,
                                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
            }
            else
            {
                vector<double> pos;
                VectorFliedd data(pos, 3);
                container->appendData("pos", data,
                                      DECAF_NOFLAG, DECAF_PRIVATE,
                                      DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);
            }

            decaf->put(container);
        }
        delete[] x;
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "lammps terminating\n");
    decaf->terminate();

    delete lps;
}

// gets the atom positions and prints them
void print(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {
        // get the values
        for (size_t i = 0; i < in_data.size(); i++)
        {
            VectorFliedd pos = in_data[i]->getFieldData<VectorFliedd>("pos");
            if (pos)
            {
                // debug
                fprintf(stderr, "consumer print1 or print3 printing %d atoms\n",
                        pos.getNbItems());
                for (int i = 0; i < 10; i++)               // print first few atoms
                    fprintf(stderr, "%.3lf %.3lf %.3lf\n",
                            pos.getVector()[3 * i],
                            pos.getVector()[3 * i + 1],
                            pos.getVector()[3 * i + 2]);
            }
            else
                fprintf(stderr, "Error: null pointer in node2\n");
        }
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "print terminating\n");
    decaf->terminate();
}

// forwards the atom positions in this example
// in a more realistic example, could filter them and only forward some subset of them
void print2(Decaf* decaf)
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {

        // get the values and add them
        for (size_t i = 0; i < in_data.size(); i++)
        {
            fprintf(stderr, "print2 forwarding positions\n");
            decaf->put(in_data[i]);
        }
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "print2 terminating\n");
    decaf->terminate();
}

extern "C"
{
    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               pConstructData in_data)              // input data
    {
        dataflow->put(in_data, DECAF_LINK);
    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int lammps_nsteps,                  // number of lammps timesteps to execute
         int analysis_interval,              // number of lammps timesteps to skip analyzing
         string infile)                      // lammps input config file
{
    MPI_Init(NULL, NULL);
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run workflow node tasks
    if (decaf->my_node("lammps"))
        lammps(decaf, lammps_nsteps, analysis_interval, infile);
    if (decaf->my_node("print"))
        print(decaf);
    if (decaf->my_node("print2"))
        print2(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

int main(int argc,
         char** argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "lammps.json");

    int lammps_nsteps     = 1;
    int analysis_interval = 1;
    char * prefix         = getenv("DECAF_PREFIX");
    if (prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/lammps/mod_lammps.so"));
    string infile = argv[1];

    run(workflow, lammps_nsteps, analysis_interval, infile);

    return 0;
}
