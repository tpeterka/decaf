//---------------------------------------------------------------------------
//
// generates synthetic particles and sends them to the rest of the workflow
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <string.h>

#include "tess/tess.h"
#include "tess/tess.hpp"

using namespace std;

// producer
void prod()
{
    // 1 block per process in this example
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int tot_blocks    = size;                      // total number of blocks in the domain
    int mem_blocks    = -1;                     // max blocks in memory
    int wrap          = 0;                      // wraparound neighbors flag
    int num_threads   = 1;                      // threads diy can use
    char outfile[256];                          // output file name
    strcpy(outfile, "del.out");
    double times[TESS_MAX_TIMES];               // timing

    // data extents
    typedef     diy::ContinuousBounds         Bounds;
    Bounds domain;
    for(int i = 0; i < 3; i++)
    {
        domain.min[i] = 0.0;
        domain.max[i] = 10.0;
    }

    // init diy
    diy::mpi::communicator    world(MPI_COMM_WORLD);
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
    AddAndGenerate            create(master, 1.0);

    // decompose
    diy::RegularDecomposer<Bounds>::BoolVector          wraps;
    diy::RegularDecomposer<Bounds>::BoolVector          share_face;
    diy::RegularDecomposer<Bounds>::CoordinateVector    ghosts;
    if (wrap)
        wraps.assign(3, true);
    diy::decompose(3, world.rank(), domain, assigner, create, share_face, wraps, ghosts);

    fprintf(stderr, "before tess: rank=%d size=%d\n", world.rank(), world.size());

    // tessellate
    quants_t quants;
    timing(times, -1, -1, world);
    timing(times, TOT_TIME, -1, world);
    tess(master, quants, times);

    // timing stats
    timing(times, -1, TOT_TIME, world);
    tess_stats(master, quants, times);
}

int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);
    prod();
    MPI_Finalize();

    return 0;
}
