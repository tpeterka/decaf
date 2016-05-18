//---------------------------------------------------------------------------
//
// tessellation module
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "wflow.hpp"                         // defines the workflow for this example
#include "tess/tess.h"
#include "tess/tess.hpp"

// using namespace decaf;
using namespace std;

// consumer
void tessellate(Decaf* decaf, MPI_Comm comm)
{
    // this first test just creates and tessellates some synthetic data to demostrate that
    // tess (a diy program) can be run as a decaf node task

    int tot_blocks = 8;                       // total number of blocks in the domain
    int mem_blocks = -1;                      // max blocks in memory
    int dsize[3] = {11, 11, 11};              // domain grid size
    float jitter = 2.0;                       // max amount to randomly displace particles
    int wrap = 0;                             // wraparound neighbors flag
    char outfile[256];                        // output file name
    int num_threads = 1;                      // threads diy can use
    strcpy(outfile, "del.out");
    double times[TESS_MAX_TIMES];             // timing

    // data extents
    typedef     diy::ContinuousBounds         Bounds;
    Bounds domain;
    for(int i = 0; i < 3; i++)
    {
        domain.min[i] = 0;
        domain.max[i] = dsize[i] - 1.0;
    }

    // init diy
    diy::mpi::communicator    world(comm);
    diy::FileStorage          storage("./DIY.XXXXXX");
    diy::Master               master(world,
                                     num_threads,
                                     mem_blocks,
                                     &create_block,
                                     &destroy_block,
                                     &storage,
                                     &save_block,
                                     &load_block);
    diy::RoundRobinAssigner   assigner(world.size(), tot_blocks);
    AddAndGenerate            create(master, jitter);

    // decompose
    std::vector<int> my_gids;
    assigner.local_gids(world.rank(), my_gids);
    diy::RegularDecomposer<Bounds>::BoolVector          wraps;
    diy::RegularDecomposer<Bounds>::BoolVector          share_face;
    diy::RegularDecomposer<Bounds>::CoordinateVector    ghosts;
    if (wrap)
        wraps.assign(3, true);
    diy::decompose(3, world.rank(), domain, assigner, create, share_face, wraps, ghosts);

    // tessellate
    quants_t quants;
    timing(times, -1, -1, world);
    timing(times, TOT_TIME, -1, world);
    tess(master, quants, times);

    // output
    tess_save(master, outfile, times);
    timing(times, -1, TOT_TIME, world);
    tess_stats(master, quants, times);

    // TODO: get input data
    // vector< shared_ptr<ConstructData> > in_data;
    // while (decaf->get(in_data))
    // {
    //     // TODO: process input data
    // }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "consumer terminating\n");
    decaf->terminate();
}

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)
{
    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // start the task
    tessellate(decaf, decaf->con_comm_handle());

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    // run decaf
    run(workflow);

    return 0;
}
