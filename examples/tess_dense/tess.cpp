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

#include <diy/serialization.hpp>

using namespace std;

// a light copy of small data (quantities, bounding boxes, etc) and pointer to the heavy data
// (particles, tets)
void copy_block(SerBlock* dest, dblock_t* src, diy::Master& master, int lid)
{

#if 0 // this version is a shallow copy of the heavy data items, but more verbose programming

    dest->gid                  = src->gid;
    memcpy(dest->mins,         src->mins,                 3 * sizeof(float));
    memcpy(dest->maxs,         src->maxs,                 3 * sizeof(float));
    memcpy(&dest->box,         &src->box,                 sizeof(bb_c_t));
    memcpy(&dest->data_bounds, &src->data_bounds,         sizeof(bb_c_t));
    dest->num_orig_particles   = src->num_orig_particles;
    dest->num_particles        = src->num_particles;
    dest->particles            = src->particles;          // shallow copy, pointer only
    dest->rem_gids             = src->rem_gids;           // shallow
    dest->rem_lids             = src->rem_lids;           // shallow
    dest->complete             = src->complete;
    dest->num_tets             = src->num_tets;
    dest->tets                 = src->tets;               // shallow
    dest->vert_to_tet          = src->vert_to_tet;        // shallow

#else  // or this version is a deep copy of all items, but easier to program

    // copy (deep) block to binary buffer
    diy::MemoryBuffer bb;
    save_block_light(master.block(lid), bb);
    dest->diy_bb.resize(bb.buffer.size());
    swap(bb.buffer, dest->diy_bb);

 #endif

    // copy (deep) link to binary buffer
    diy::MemoryBuffer lb;
    diy::LinkFactory::save(lb, master.link(lid));
    dest->diy_lb.resize(lb.buffer.size());
    swap(lb.buffer, dest->diy_lb);

    // debug
    // fprintf(stderr, "tess: lid=%d gid=%d\n", lid, src->gid);
    // fprintf(stderr, "mins [%.3f %.3f %.3f] maxs [%.3f %.3f %.3f]\n",
    //         src->mins[0], src->mins[1], src->mins[2],
    //         src->maxs[0], src->maxs[1], src->maxs[2]);
    // fprintf(stderr, "box min [%.3f %.3f %.3f] max [%.3f %.3f %.3f]\n",
    //         src->box.min[0], src->box.min[1], src->box.min[2],
    //         src->box.max[0], src->box.max[1], src->box.max[2]);
    // fprintf(stderr, "data_bounds min [%.3f %.3f %.3f] max [%.3f %.3f %.3f]\n",
    //         src->data_bounds.min[0], src->data_bounds.min[1], src->data_bounds.min[2],
    //         src->data_bounds.max[0], src->data_bounds.max[1], src->data_bounds.max[2]);
    // fprintf(stderr, "num_orig_particles %d num_particles %d\n",
    //         src->num_orig_particles, src->num_particles);
    // fprintf(stderr, "complete %d num_tets %d\n", src->complete, src->num_tets);

    // fprintf(stderr, "particles:\n");
    // for (int j = 0; j < src->num_particles; j++)
    //     fprintf(stderr, "gid=%d j=%d [%.3f %.3f %.3f] ", src->gid, j,
    //             src->particles[3 * j], src->particles[3 * j + 1], src->particles[3 * j + 2]);
    // fprintf(stderr, "\n");

    // fprintf(stderr, "rem_gids:\n");
    // for (int j = 0; j < src->num_particles - src->num_orig_particles; j++)
    //     fprintf(stderr, "gid=%d j=%d rem_gid=%d ", src->gid, j, src->rem_gids[j]);
    // fprintf(stderr, "\n");

    // fprintf(stderr, "rem_lids:\n");
    // for (int j = 0; j < src->num_particles - src->num_orig_particles; j++)
    //     fprintf(stderr, "gid=%d j=%d rem_lid=%d ", src->gid, j, src->rem_lids[j]);
    // fprintf(stderr, "\n");

    // fprintf(stderr, "tets:\n");
    // for (int j = 0; j < src->num_tets; j++)
    //     fprintf(stderr, "gid=%d tet[%d]=[%d %d %d %d; %d %d %d %d] ",
    //             src->gid, j,
    //             src->tets[j].verts[0], src->tets[j].verts[1],
    //             src->tets[j].verts[2], src->tets[j].verts[3],
    //             src->tets[j].tets[0], src->tets[j].tets[1],
    //             src->tets[j].tets[2], src->tets[j].tets[3]);
    // fprintf(stderr, "\n");

    // fprintf(stderr, "vert_to_tet:\n");
    // for (int j = 0; j < src->num_particles; j++)
    //     fprintf(stderr, "gid=%d vert_to_tet[%d]=%d ", src->gid, j, src->vert_to_tet[j]);
    // fprintf(stderr, "\n");
}

// consumer
void tessellate(Decaf* decaf, MPI_Comm comm)
{
    static int step = 0;

    // event loop
    // TODO: verify that all of the below can run iteratively, only tested for one time step
    vector<pConstructData> in_data;
    while (decaf->get(in_data))
    {
        fprintf(stderr, "tess: getting data for step %d\n", step);

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

        // debug: save file
        // tess_save(master, outfile, times);

        // timing stats
        timing(times, -1, TOT_TIME, world);
        tess_stats(master, quants, times);

        // fill containers for the blocks and send them
        pConstructData container;

        vector<SerBlock> ser_blocks(master.size());
        for (size_t i = 0; i < master.size(); i++)
            copy_block(&ser_blocks[i], (dblock_t*)master.block(i), master, i);

        ArrayField<SerBlock> blocks_array(&ser_blocks[0], master.size(), 1, false);
        container->appendData(string("blocks_array"), blocks_array,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        decaf->put(container);

        step++;
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
    // make_wflow(workflow);
    Workflow::make_wflow_from_json(workflow, "tess_dense.json");

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
