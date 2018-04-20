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

void detect(
        Decaf* decaf,
        string outfile)         // currently unused but will be needed once detector write output files
{
    vector< pConstructData > in_data;

    while (decaf->get(in_data))
    {
        // get the values
        for (size_t i = 0; i < in_data.size(); i++)
        {
            VectorFliedd pos = in_data[i]->getFieldData<VectorFliedd>("pos");
//             if (pos)
//             {
                // debug
//                 fprintf(stdout, "detector received %d atoms; printing first 10:\n",
//                         pos.getNbItems());
//                 for (int i = 0; i < 10; i++)               // print first few atoms
//                     fprintf(stdout, "%.3lf %.3lf %.3lf\n",
//                             pos.getVector()[3 * i],
//                             pos.getVector()[3 * i + 1],
//                             pos.getVector()[3 * i + 2]);
//             }
//             else
//                 fprintf(stdout, "Error: null pointer in detector\n");
        }
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
//     fprintf(stdout, "detector terminating\n");
    decaf->terminate();
}

int main(int    argc,
         char** argv)
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
    string outfile = argv[2];

    // init
    MPI_Init(NULL, NULL);
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run
    detect(decaf, outfile);

    // finalize
    delete decaf;
    MPI_Finalize();

    return 0;
}
