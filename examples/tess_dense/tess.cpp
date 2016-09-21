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
#include <decaf/data_model/vectorfield.hpp>
#include <decaf/data_model/blockfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <string.h>

#include "wflow.hpp"                         // defines the workflow for this example
#include "tess/tess.h"
#include "tess/tess.hpp"
#include "block_serialization.hpp"

using namespace std;

// a light copy of small data (quantities, bounding boxes, etc) and pointer to the heavy data
// (particles, tets)
void copy_block(dblock_t& dest, dblock_t* src)
{
    dest.gid                  = src->gid;
    memcpy(dest.mins,         src->mins,                 3 * sizeof(float));
    memcpy(dest.maxs,         src->maxs,                 3 * sizeof(float));
    memcpy(&dest.box,         &src->box,                 sizeof(bb_c_t));
    memcpy(&dest.data_bounds, &src->data_bounds,         sizeof(bb_c_t));
    dest.num_orig_particles   = src->num_orig_particles;
    dest.num_particles        = src->num_particles;
    dest.particles            = src->particles;          // shallow copy, pointer only
    dest.rem_gids             = src->rem_gids;           // shallow
    dest.rem_lids             = src->rem_lids;           // shallow
    dest.num_grid_pts         = src->num_grid_pts;
    dest.density              = src->density;            // shallow
    dest.complete             = src->complete;
    dest.num_tets             = src->num_tets;
    dest.tets                 = src->tets;               // shallow
    dest.vert_to_tet          = src->vert_to_tet;        // shallow
}

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
            dblock_t* b = (dblock_t*)master.block(i);
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
        // TODO: remove the file save when sending the tessellation through the workflow instead
        tess_save(master, outfile, times);
        timing(times, -1, TOT_TIME, world);
        tess_stats(master, quants, times);

        // fill containers for the blocks and send them
        pConstructData container;
        vector<dblock_t> blocks(master.size());
        for (size_t i = 0; i < master.size(); i++)
        {
            copy_block(blocks[i], (dblock_t*)master.block(i));
            ArrayField<dblock_t> blocks_array(&blocks[i], master.size(), 1, false);
            container->appendData(string("blocks_array"), blocks_array,
                                  DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        }
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
