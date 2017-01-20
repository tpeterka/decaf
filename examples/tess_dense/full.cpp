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
    dest->diy_bb.resize(bb.buffer.size());
    swap(bb.buffer, dest->diy_bb);

 #endif

    // copy (deep) link to binary buffer
    diy::MemoryBuffer lb;
    diy::LinkFactory::save(lb, master.link(lid));
    dest->diy_lb.resize(lb.buffer.size());
    swap(lb.buffer, dest->diy_lb);
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
        fprintf(stderr, "gid=%d found %d particles that appear more than once, with %d "
                " total extra copies\n", b->gid, count.size(), total);
    }
}

// producer
void prod()
{

/************** PROD code *************************/
    // hacc produces 3 arrays of position coordinates
    // we're making up some synthetic data instead

    // number of points, randomly placed in a box of size [0, npts - 1]^3
    int npts = 20;

    // add 1 to the rank because on some compilers, srand(0) = srand(1)
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    srand(my_rank + 1);

    // produce data for some number of timesteps
    for (int timestep = 0; timestep < 1; timestep++)
    {
        // create the synthetic points for the timestep
        fprintf(stderr, "producer timestep %d\n", timestep);

        // the point arrays need to be allocated inside the loop because
        // decaf will automatically free them at the end of the loop after sending
        // (smart pointer reference counting)
        float* x = new float[npts];
        float* y = new float[npts];
        float* z = new float[npts];
        for (unsigned i = 0; i < npts; ++i)
        {
            // debug: test what happens when a point is duplicated or outside the domain
            // if (i == 1)                           // duplicate point test
            // {
            //     x[i] = x[i - 1];
            //     y[i] = y[i - 1];
            //     z[i] = z[i - 1];
            // }
            // else if (i == npts  - 1)              // out of bounds test
            // {
            //     float t = (float) rand() / RAND_MAX;
            //     x[i] = npts;
            //     t = (float) rand() / RAND_MAX;
            //     y[i] = t * (npts - 1);
            //     t = (float) rand() / RAND_MAX;
            //     z[i] = t * (npts - 1);
            // }
            // else
            // {
                float t = (float) rand() / RAND_MAX;
                x[i] = t * (npts - 1);
                t = (float) rand() / RAND_MAX;
                y[i] = t * (npts - 1);
                t = (float) rand() / RAND_MAX;
                z[i] = t * (npts - 1);
            // }
        }

        // mins and sizes, not mins and maxs
        vector<unsigned int> extendsBlock = {0, 0, 0,
                                             (unsigned)npts, (unsigned)npts, (unsigned)npts};
        vector<float> bboxBlock = {0.0, 0.0, 0.0, (float)npts, float(npts), float(npts)};

        /************** DFLOW1 code *************************/

        int nbParticleDflow = npts;

        float* xyzpos = new float[nbParticleDflow *3];
        for (int i = 0; i < nbParticleDflow; i++)
        {
            xyzpos[3 * i    ] = x[i];
            xyzpos[3 * i + 1] = y[i];
            xyzpos[3 * i + 2] = z[i];
            // debug
            // fprintf(stderr, "%.3f %.3f %.3f\n", xyzpos[3*i], xyzpos[3*i+1], xyzpos[3*i+2]);
        }

        /************** TESS code *************************/



        // 1 block per process in this example
        int size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        int tot_blocks    = size;                      // total number of blocks in the domain

        float* xyz        = xyzpos;                   // points
        //Block<3>* blk     = domainData.getBlock();       // domain and block info
        //float* global_box = blk->getGlobalBBox();   // min and size (not min and max)
        //float* local_box  = blk->getLocalBBox();    // min and size (not min and size)
        float* global_box = &bboxBlock[0];
        int nbParticle    = nbParticleDflow;   // number of particles in this process
        int mem_blocks    = -1;                     // max blocks in memory
        int wrap          = 0;                      // wraparound neighbors flag
        int num_threads   = 1;                      // threads diy can use
        char outfile[256];                          // output file name
        strcpy(outfile, "del.out");
        double times[TESS_MAX_TIMES];               // timing

        fprintf(stderr,"Size of the communicator: %i\n", size);

        // data extents
        typedef     diy::ContinuousBounds         Bounds;
        Bounds domain;
        for(int i = 0; i < 3; i++)
        {
            domain.min[i] = global_box[i];
            domain.max[i] = global_box[i] + global_box[3 + i] - 1.0;
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
        AddEmpty                  create(master);

        int size_comm_diy;
        MPI_Comm_size(world, &size_comm_diy);
        int rank_diy;
        MPI_Comm_rank(world, &rank_diy);

        fprintf(stderr,"DIY size communicator: %i\n", size_comm_diy);
        fprintf(stderr,"DIY rank communicator: %i\n", rank_diy);

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

        DBlock* b = (DBlock*)master.block(0);

        fprintf(stderr, "Number of blocks in the master: %u\n", master.size());
        fprintf(stderr, "Number of particles in the block: %i\n", b->num_orig_particles);

        MPI_Comm_size(world, &size_comm_diy);
        MPI_Comm_rank(world, &rank_diy);

        fprintf(stderr,"DIY size communicator before TESS: %i\n", size_comm_diy);
        fprintf(stderr,"DIY rank communicator before TESS: %i\n", rank_diy);
        fprintf(stderr,"Retrieving the communicator from DIY...\n");
        MPI_Comm_rank(master.communicator(), &rank_diy);
        fprintf(stderr,"DIY rank from master communicator before TESS: %i\n", rank_diy);


        // sort and distribute particles to regular blocks
        tess_exchange(master, assigner, times);

        fprintf(stderr,"Tesselation exchange done\n");

        // clean up any duplicates and out of bounds particles
        DuplicateCountMap count;
        master.foreach([&](DBlock* b, const diy::Master::ProxyWithLink& cp)
                       { deduplicate(b, cp, count); });

        fprintf(stderr, "Removed the duplicated particles\n");

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


        // send the data on all outbound dataflows
        // in this example there is only one outbound dataflow, but in general there could be more
        //decaf->put(container);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    //fprintf(stderr, "producer terminating\n");
    //decaf->terminate();
}

int main(int argc,
         char** argv)
{
    // define the workflow
    //Workflow workflow;
    // make_wflow(workflow);
    //Workflow::make_wflow_from_json(workflow, "tess_dense.json");

    MPI_Init(NULL, NULL);

    // create decaf
    //Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // timing
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();
    if (my_rank == 0)
        fprintf(stderr, "*** workflow starting time = %.3lf s. ***\n", t0);

    // start the task
    prod();

    // timing
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_rank == 0)
        fprintf(stderr, "*** prod. elapsed time = %.3lf s. ***\n", MPI_Wtime() - t0);

    // cleanup
    //delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished prod\n");
    return 0;
}
