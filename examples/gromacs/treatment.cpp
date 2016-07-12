//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
// this file contains the consumer (2 procs)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/blockfield.hpp>
//#include <decaf/data_model/array3dconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <decaf/data_model/boost_macros.h>

#include "decaf/data_model/morton.h"

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "wflow_gromacs.hpp"                         // defines the workflow for this example

using namespace decaf;
using namespace std;
using namespace boost;

RedistBlockMPI* redist;

// consumer
void treatment1(Decaf* decaf)
{
    vector< pConstructData > in_data;
    fprintf(stderr, "Launching treatment\n");
    fflush(stderr);
    while (decaf->get(in_data))
    {
        // get the atom positions
        pConstructData atoms;
        fprintf(stderr, "Reception of %u messages.\n", in_data.size());
        if(in_data[0]->hasData(string("domain_block")))
            fprintf(stderr, "The block is in the model before block redist\n");
        else
            fprintf(stderr, "The block is not in the model before block redist\n");

        for (size_t i = 0; i < in_data.size(); i++)
        {
            fprintf(stderr, "Number of particles received : %i\n", in_data[i]->getNbItems());

            // Sending the data
            redist->process(in_data[i], DECAF_REDIST_SOURCE);

            //Receiving the data
            redist->process(atoms, DECAF_REDIST_DEST);
            redist->flush();
        }
        atoms->printKeys();

        // Now each process has a sub domain of the global grid

        // Getting the grid info
        /*BlockField blockField  = atoms->getFieldData<BlockField>("domain_block");
        Block<3>* block = blockField.getBlock();
        block->printExtends();

        //Building the grid
        unsigned int* lExtends = block->getLocalExtends();
        multi_array<float,3>* grid = new multi_array<float,3>(
                    extents[lExtends[3]][lExtends[4]][lExtends[5]]
                );


        ArrayFieldu mortonField = atoms->getFieldData<ArrayFieldu>("morton");
        unsigned int *morton = mortonField.getArray();
        int nbMorton = mortonField->getNbItems();

        for(int i = 0; i < nbMorton; i++)
        {
            unsigned int x,y,z;
            Morton_3D_Decode_10bit(morton[i], x, y, z);

            // Checking if the particle should be here
            if(!block->isInLocalBlock(x,y,z))
                fprintf(stderr, "ERROR : particle not belonging to the local block. FIXME\n");

            unsigned int localx, localy, localz;
            localx = x - lExtends[0];
            localy = y - lExtends[1];
            localz = z - lExtends[2];

            (*grid)[localx][localy][localz] += 1.0f; // TODO : get the full formulation
        }


        delete grid;
        */
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "Treatment terminating\n");
    decaf->terminate();
}

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                             // workflow
{
    MPI_Init(NULL, NULL);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "treatment rank %i\n", rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);
    fprintf(stderr, "Decaf created\n");

    // We do the redistribution here between the component
    // TODO : Insert this in the workflow instead
    fprintf(stderr, "Size of the communicator for block redist : %i\n", decaf->con_comm_size());
    redist = new RedistBlockMPI(0, decaf->con_comm_size(),
                          0, decaf->con_comm_size(), 20,
                          decaf->con_comm_handle());

    // start the task
    treatment1(decaf);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    fprintf(stderr, "Hello treatment\n");
    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    // run decaf
    run(workflow);

    return 0;
}
