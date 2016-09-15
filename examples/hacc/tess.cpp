//---------------------------------------------------------------------------
//
// tessellation
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
    // event loop
    // TODO: verify that all of the below can run iteratively, only tested for one time step
    vector<pConstructData> in_data;
    while (decaf->get(in_data))
    {

        ArrayFieldf xyzpos = in_data[0]->getFieldData<ArrayFieldf>("xyz_pos");
        if(!xyzpos)
        {
            fprintf(stderr, "ERROR tessellate: unable to find the field required \"xyz_pos\" "
                    "in the data model.\n");
            return;
        }

        BlockField block = in_data[0]->getFieldData<BlockField>("domain_block");
        if(!block)
        {
            fprintf(stderr, "ERROR tessellate: unable to find the field required \"domain_block\" "
                    "in the data model.\n");
            return;
        }

        float* xyz        = xyzpos.getArray();      // points
        Block<3>* blk     = block.getBlock();       // domain and block info
        float* global_box = blk->getGlobalBBox();   // min and size (not min and max)
        float* local_box  = blk->getLocalBBox();    // min and size (not min and size)
        int nbParticle    = xyzpos->getNbItems();   // number of particles in this process
        int tot_blocks    = 8;                      // total number of blocks in the domain
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
            domain.min[i] = global_box[i];
            domain.max[i] = global_box[i] + global_box[3 + i] - 1.0;
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
        AddBlock                  create(master);

        // decompose
        std::vector<int> my_gids;
        assigner.local_gids(world.rank(), my_gids);
        diy::RegularDecomposer<Bounds>::BoolVector          wraps;
        diy::RegularDecomposer<Bounds>::BoolVector          share_face;
        diy::RegularDecomposer<Bounds>::CoordinateVector    ghosts;
        if (wrap)
            wraps.assign(3, true);
        diy::decompose(3, world.rank(), domain, assigner, create, share_face, wraps, ghosts);

        // put all the points into the first local block
        // tess will sort them among the blocks anyway
        for (size_t i = 0; i < master.size(); i++)
        {
            dblock_t* b = (dblock_t*)(master.block(i));
            if (i == 0)
            {
                b->particles = (float *)malloc(nbParticle * 3 * sizeof(float));
                b->num_orig_particles = b->num_particles = nbParticle;
                for (int i = 0; i < 3 * nbParticle; i++)
                    b->particles[i] = xyz[i];
            }
            // all blocks, even the empty ones need the box bounds set prior to doing the exchange
            for (int i = 0; i < 3; ++i)
            {
                b->box.min[i] = domain.min[i];
                b->box.max[i] = domain.max[i];
            }
        }


        // sort and distribute particles to regular blocks
        tess_exchange(master, assigner, times);

        // tessellate
        quants_t quants;
        timing(times, -1, -1, world);
        timing(times, TOT_TIME, -1, world);
        tess(master, quants, times);

        // output
        tess_save(master, outfile, times);
        timing(times, -1, TOT_TIME, world);
        tess_stats(master, quants, times);

        // send a token downstream that data are ready
        // TODO: send data instead of the token (don't write to a file)
        fprintf(stderr, "tess sending token\n");
        int value = 0;

        SimpleFieldi data(value);
        pConstructData container;
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
        decaf->put(container);
    } // decaf event loop

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "tessellation terminating\n");
    decaf->terminate();
}

int main(int argc,
         char** argv)
{
    // define the workflow
    Workflow workflow;
    make_wflow(workflow);

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // start the task
    tessellate(decaf, decaf->con_comm_handle());

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished prod\n");
    return 0;
}
