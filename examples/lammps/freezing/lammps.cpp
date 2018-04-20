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
#include "update.h"

using namespace decaf;
using namespace LAMMPS_NS;
using namespace std;

// runs lammps and puts the atom positions to the dataflow at the consumer intervals
void lammps(
        Decaf*  decaf,
        string  infile)
{
    LAMMPS* lps = new LAMMPS(0, NULL, decaf->prod_comm_handle());
    lps->input->file(infile.c_str());

    int nsteps = lps->update->nsteps;
    for (int timestep = 0; timestep < nsteps; timestep++)
    {
        lps->input->one("run 1");
        int natoms = static_cast<int>(lps->atom->natoms);
        double* x = new double[3 * natoms];
        lammps_gather_atoms(lps, (char*)"x", 1, 3, x);

        pConstructData container;

        // lammps gathered all positions to rank 0
        if (decaf->prod_comm()->rank() == 0)
        {
//             fprintf(stdout, "lammps sending time step %d with %d atoms to detector\n", timestep, natoms);
            // debug
            //         for (int i = 0; i < 10; i++)         // print first few atoms
            //           fprintf(stdout, "%.3lf %.3lf %.3lf\n",
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
        delete[] x;
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
//     fprintf(stdout, "lammps terminating\n");
    decaf->terminate();

    delete lps;
}

int main(int    argc,
        char**  argv)
{
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "lammps.json");
    char * prefix         = getenv("DECAF_PREFIX");
    if (prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }
    string infile = argv[1];

    // init
    MPI_Init(NULL, NULL);
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run
    lammps(decaf, infile);

    // finalize
    delete decaf;
    MPI_Finalize();

    return 0;
}
