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
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/blockfield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <string.h>

#include "tess/tess.h"
#include "tess/tess.hpp"
#include "block_serialization.hpp"

using namespace decaf;
using namespace std;

// a light copy of small data (quantities, bounding boxes, etc) and pointer to the heavy data
// (particles, tets)
void copy_block(SerBlock* dest, DBlock* src, diy::Master& master, int lid)
{

#if 0 // this version is a shallow copy of the heavy data items, but more verbose programming

    dest->gid                  = src->gid;
    dest->bounds               = src->bounds;
    dest->box                  = src->src->box;
    dest->data_bounds          = src->data_bounds;
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
    fprintf(stderr, "Serializing %i origin particles.\n", ((DBlock*)master.block(lid))->num_orig_particles);
    fprintf(stderr, "Serializing %i with ghost particles.\n", ((DBlock*)master.block(lid))->num_particles);
    //dest->diy_bb.resize(bb.buffer.size());
    //swap(bb.buffer, dest->diy_bb);
    //dest->diy_bb.reserve(bb.buffer.size());
    //copy(bb.buffer.begin(), bb.buffer.end(), dest->diy_bb.begin());
    dest->diy_bb = vector<char>(bb.buffer);

 #endif

    // copy (deep) link to binary buffer
    diy::MemoryBuffer lb;
    diy::LinkFactory::save(lb, master.link(lid));
    //dest->diy_lb.resize(lb.buffer.size());
    //swap(lb.buffer, dest->diy_lb);
    //dest->diy_lb.reserve(lb.buffer.size());
    //copy(lb.buffer.begin(), lb.buffer.end(), dest->diy_lb.begin());
    dest->diy_lb = vector<char>(lb.buffer);

    // debug
    // fprintf(stderr, "tess: lid=%d gid=%d\n", lid, src->gid);
    // fprintf(stderr, "mins [%.3f %.3f %.3f] maxs [%.3f %.3f %.3f]\n",
    //         src->bounds.min[0], src->bounds.min[1], src->bounds.min[2],
    //         src->bounds.max[0], src->bounds.max[1], src->bounds.max[2]);
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

//
// Deduplicate functionality: get rid of duplicate points, since downstream code can't handle them.
//
typedef     std::map<size_t, int>       DuplicateCountMap;

struct DedupPoint
{
    float data[3];
    bool	    operator<(const DedupPoint& other) const
        { return std::lexicographical_compare(data, data + 3, other.data, other.data + 3); }
    bool	    operator==(const DedupPoint& other) const
        { return std::equal(data, data + 3, other.data); }
};
void deduplicate(DBlock*                           b,
                 const diy::Master::ProxyWithLink& cp,
                 DuplicateCountMap&                count)
{
    // debug
    // for (int i = 0; i < b->num_particles; i++)
    //     fprintf(stderr, "%.3f %.3f %.3f\n", 
    //             b->particles[3 * i], b->particles[3 * i + 1], b->particles[3 * i + 2]);

    if (!b->num_particles)
        return;

    // simple static_assert to ensure sizeof(Point) == sizeof(float[3]);
    // necessary to make this hack work
    typedef int static_assert_Point_size[sizeof(DedupPoint) == sizeof(float[3]) ? 1 : -1];
    DedupPoint* bg  = (DedupPoint*) &b->particles[0];
    DedupPoint* end = (DedupPoint*) &b->particles[3*b->num_particles];
    std::sort(bg,end);

    DedupPoint* out = bg + 1;
    for (DedupPoint* it = bg + 1; it != end; ++it)
    {
        if (*it == *(it - 1))
            count[out - bg - 1]++;
        else
        {
            *out = *it;
            ++out;
        }
    }
    b->num_orig_particles = b->num_particles = out - bg;

    if (!count.empty())
    {
        size_t total = 0;
        for (DuplicateCountMap::const_iterator it = count.begin(); it != count.end(); ++it)
            total += it->second;
        fprintf(stderr, "gid=%d found %ld particles that appear more than once, with %ld "
                " total extra copies\n", b->gid, count.size(), total);
    }
}

// consumer
void tessellate(
        Decaf*      decaf,
        int         tot_blocks,
        MPI_Comm    comm)
{
    static int step = 0;

    // event loop
    // TODO: verify that all of the below can run iteratively, only tested for one time step
    vector<pConstructData> in_data;
    while (decaf->get(in_data))
    {
        fprintf(stderr, "tess: getting data for step %d\n", step);

        //fprintf(stderr, "Size of my consumer on outbound 0: %i\n", decaf->con_comm_size(0));

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
        AddEmpty                  create(master);

        // decompose
        diy::RegularDecomposer<Bounds>::BoolVector          wraps;
        diy::RegularDecomposer<Bounds>::BoolVector          share_face;
        diy::RegularDecomposer<Bounds>::CoordinateVector    ghosts;
        if (wrap)
            wraps.assign(3, true);
        diy::decompose(3, world.rank(), domain, assigner, create, share_face, wraps, ghosts);

        // debug
        // for (int i = 0; i < nbParticle; i++)
        //     fprintf(stderr, "%.3f %.3f %.3f\n",
        //            xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]);

        // put all the points into the first local block
        // tess will sort them among the blocks anyway
        // filter out any points out of the global domain
        for (size_t k = 0; k < master.size(); k++)
        {
            DBlock* b = (DBlock*)master.block(k);

            if (k == 0)
            {
                b->particles = (float *)malloc(nbParticle * 3 * sizeof(float));
                // count number of "good" particles (in bounds)
                b->num_orig_particles = 0;
                for (int i = 0; i < nbParticle; i++)
                {
                    bool skip = false;
                    for (int j = 0; j < 3; ++j)
                    {
                        if (xyz[3 * i + j] < domain.min[j] || xyz[3 * i + j] > domain.max[j])
                        {
                            // fprintf(stderr, "skipping particle[%d]=%.3f\n", i, xyz[3 * i + j]);
                            skip = true;
                            break;
                        }
                    }
                    if (!skip)
                    {
                        for (int j = 0; j < 3; ++j)
                            b->particles[3 * b->num_orig_particles + j] = xyz[3 * i + j];
                        b->num_orig_particles++;
                    }
                }
                b->num_particles = b->num_orig_particles;
            }
            // realloc in case some particles were not used
            if (b->num_orig_particles != nbParticle)
                b->particles = (float *)realloc(b->particles,
                                                b->num_orig_particles * 3 * sizeof(float));

            // all blocks, even the empty ones need the box bounds set prior to doing the exchange
            for (int i = 0; i < 3; ++i)
            {
                b->box.min[i] = domain.min[i];
                b->box.max[i] = domain.max[i];
            }
        }

        // sort and distribute particles to regular blocks
        tess_exchange(master, assigner, times);

        // debug
        // fprintf(stderr,"Tesselation exchange done\n");

        // clean up any duplicates and out of bounds particles
        DuplicateCountMap count;
        master.foreach([&](DBlock* b, const diy::Master::ProxyWithLink& cp)
                       { deduplicate(b, cp, count); });

        // debug
        // fprintf(stderr, "Removed the duplicated particles\n");

        // tessellate
        quants_t quants;
        timing(times, -1, -1, world);
        timing(times, TOT_TIME, -1, world);
        tess(master, quants, times);

        // debug: save file
        tess_save(master, outfile, times);

        // timing stats
        timing(times, -1, TOT_TIME, world);
        tess_stats(master, quants, times);

        // fill containers for the blocks and send them
        pConstructData container;

        // Create empty vector
        VectorField<SerBlock> blocks_vector(1, 0);

        vector<SerBlock>& ser_blocks = blocks_vector.getVector();
        ser_blocks.resize(master.size());

        for (size_t i = 0; i < master.size(); i++)
            copy_block(&ser_blocks[i], (DBlock*)master.block(i), master, i);

        container->appendData(string("blocks_array"), blocks_vector,
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
    MPI_Init(NULL, NULL);

    if(argc < 4)
    {
        fprintf(stderr, "Usage: tess <unused> <unused> tot_blocks\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    int tot_blocks = atoi(argv[3]);
    fprintf(stderr,"Generating %i blocks total.\n", tot_blocks);

    // define the workflow
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "tess_dense.json");

    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    fprintf(stderr,"My rank: %i\n", my_rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // timing
    MPI_Barrier(decaf->con_comm_handle());
    double t0 = MPI_Wtime();

    // start the task
    tessellate(decaf, tot_blocks, decaf->con_comm_handle());

    // timing
    MPI_Barrier(decaf->con_comm_handle());
    if (decaf->con_comm()->rank() == 0)
        fprintf(stderr, "*** tess elapsed time = %.3lf s. ***\n", MPI_Wtime() - t0);

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished prod\n");
    return 0;
}
