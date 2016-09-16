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

#include "wflow.hpp"                         // defines the workflow for this example
#include "tess/tess.h"
#include "tess/tess.hpp"

// using namespace decaf;
using namespace std;

void fill_container(pConstructData& c,       // decaf container
                    diy::Master& master)     // diy master
{
    // TODO: only doing first block for now; need to learn how to send multiple blocks
    dblock_t* b = (dblock_t*)master.block(0);

    SimpleFieldi gid(b->gid);
    c->appendData(string("gid"), gid,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf mins(b->mins, 3, 1, false); // true = decaf will free; TODO: crashes when true
    c->appendData(string("mins"), mins,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf maxs(b->mins, 3, 1, false);
    c->appendData(string("maxs"), mins,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf box_min(b->box.min, DIY_MAX_DIM, 1, false);
    c->appendData(string("box_min"), box_min,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf box_max(b->box.max, DIY_MAX_DIM, 1, false);
    c->appendData(string("box_max"), box_max,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf data_bounds_min(b->data_bounds.min, DIY_MAX_DIM, 1, false);
    c->appendData(string("data_bounds_min"), data_bounds_min,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf data_bounds_max(b->data_bounds.max, DIY_MAX_DIM, 1, false);
    c->appendData(string("data_bounds_max"), data_bounds_max,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    SimpleFieldi num_orig_particles(b->num_orig_particles);
    c->appendData(string("num_orig_particles"), num_orig_particles,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    SimpleFieldi num_particles(b->num_particles);
    c->appendData(string("num_particles"), num_particles,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldf particles(b->particles, 3 * b->num_particles, 3, false);
    c->appendData(string("particles"), particles,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    // TODO: following causes errors
    // how does decaf know size of my array?
    // and if it knows it, then why do I need to specify it?
    // and why can't I send less than the full length?

    // ArrayFieldi rem_gids(b->rem_gids, b->num_particles - b->num_orig_particles, 1, false);
    // c->appendData(string("rem_gids"), rem_gids,
    //               DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
    // ArrayFieldi rem_lids(b->rem_lids, b->num_particles - b->num_orig_particles, 1, false);
    // c->appendData(string("rem_lids"), rem_lids,
    //               DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    SimpleFieldi num_grid_pts(b->num_grid_pts);
    c->appendData(string("num_grid_pts"), num_grid_pts,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    // num_grid_pts is 0 because density not computed yet at this stage of the workflow
    // but it doesn't cost anything to include an empty density field
    ArrayFieldf density(b->density, b->num_grid_pts, 1, false);
    c->appendData(string("density"), density,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    SimpleFieldi complete(b->complete);
    c->appendData(string("complete"), complete,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    SimpleFieldi num_tets(b->num_tets);
    c->appendData(string("num_tets"), num_tets,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    // TODO: following causes errors
    // how does decaf know how to serialize a single tet?
    // and why doesn't it like the size of my array?
    // ArrayField<tet_t> tets(b->tets, b->num_tets, 1, false);
    // c->appendData(string("tets"), tets,
    //               DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    ArrayFieldi vert_to_tet(b->vert_to_tet, b->num_particles, 1, false);
    c->appendData(string("vert_to_tet"), vert_to_tet,
                  DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    // TODO: following causes errors in number of items
    // append diy link
    // diy::Link* link = master.link(0);             // TODO, only block 0 for now
    // vector<diy::BlockID> neighbors_;
    // for (size_t i = 0; i < link->size(); i++)
    //     neighbors_.push_back(link->target(i));
    // VectorField<diy::BlockID> neighbors(neighbors_, 1);
    // c->appendData(string("neighbors"), neighbors,
    //               DECAF_NOFLAG, DECAF_PRIVATE, DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
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

        // TODO: following not out-of-core safe; should be done in foreach instead
        fill_container(container, master);

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
