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

void detect(Decaf* decaf)
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

    MPI_Init(NULL, NULL);
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    detect(decaf);

    delete decaf;
    MPI_Finalize();

    return 0;
}
