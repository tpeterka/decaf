//---------------------------------------------------------------------------
//
// density estimation
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/vectorfield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "tess/tess.h"
#include "tess/tess.hpp"
#include "tess/dense.hpp"
#include "block_serialization.hpp"

using namespace decaf;
using namespace std;

void output(DBlock* d, const diy::Master::ProxyWithLink& cp)
{
     fprintf(stderr, "dense: lid=%d gid=%d\n", 0, d->gid);
     fprintf(stderr, "mins [%.3f %.3f %.3f] maxs [%.3f %.3f %.3f]\n",
             d->bounds.min[0], d->bounds.min[1], d->bounds.min[2],
             d->bounds.max[0], d->bounds.max[1], d->bounds.max[2]);
     fprintf(stderr, "box min [%.3f %.3f %.3f] max [%.3f %.3f %.3f]\n",
             d->box.min[0], d->box.min[1], d->box.min[2],
             d->box.max[0], d->box.max[1], d->box.max[2]);
     fprintf(stderr, "data_bounds min [%.3f %.3f %.3f] max [%.3f %.3f %.3f]\n",
             d->data_bounds.min[0], d->data_bounds.min[1], d->data_bounds.min[2],
             d->data_bounds.max[0], d->data_bounds.max[1], d->data_bounds.max[2]);
     fprintf(stderr, "num_orig_particles %d num_particles %d\n",
             d->num_orig_particles, d->num_particles);
     fprintf(stderr, "complete %d num_tets %d\n", d->complete, d->num_tets);

     fprintf(stderr, "particles:\n");
     for (int j = 0; j < d->num_particles; j++)
         fprintf(stderr, "gid=%d j=%d [%.3f %.3f %.3f] ", d->gid, j,
                 d->particles[3 * j], d->particles[3 * j + 1], d->particles[3 * j + 2]);
     fprintf(stderr, "\n");

     fprintf(stderr, "rem_gids:\n");
     for (int j = 0; j < d->num_particles - d->num_orig_particles; j++)
         fprintf(stderr, "gid=%d j=%d rem_gid=%d ", d->gid, j, d->rem_gids[j]);
     fprintf(stderr, "\n");

     fprintf(stderr, "rem_lids:\n");
     for (int j = 0; j < d->num_particles - d->num_orig_particles; j++)
         fprintf(stderr, "gid=%d j=%d rem_lid=%d ", d->gid, j, d->rem_lids[j]);
     fprintf(stderr, "\n");

     fprintf(stderr, "tets:\n");
     for (int j = 0; j < d->num_tets; j++)
         fprintf(stderr, "gid=%d tet[%d]=[%d %d %d %d; %d %d %d %d] ",
                 d->gid, j,
                 d->tets[j].verts[0], d->tets[j].verts[1],
                 d->tets[j].verts[2], d->tets[j].verts[3],
                 d->tets[j].tets[0], d->tets[j].tets[1],
                 d->tets[j].tets[2], d->tets[j].tets[3]);
     fprintf(stderr, "\n");

     fprintf(stderr, "vert_to_tet:\n");
     for (int j = 0; j < d->num_particles; j++)
         fprintf(stderr, "gid=%d vert_to_tet[%d]=%d ", d->gid, j, d->vert_to_tet[j]);
     fprintf(stderr, "\n");
}

// fill blocks with incoming data
void fill_blocks(vector<pConstructData>& in_data, diy::Master& master, diy::Assigner& assigner)
{
    // only using first message in in_data (in_data[0])
    //ArrayField<SerBlock> af = in_data[0]->getFieldData<ArrayField<SerBlock> >("blocks_array");
    VectorField<SerBlock> af = in_data[0]->getFieldData<VectorField<SerBlock> >("blocks_array");
    if (af)
    {
        //SerBlock* b = af.getArray();
        vector<SerBlock>& b = af.getVector();

        for (int i = 0; i < af.getNbItems(); i++)
        {
            dblock_t* d = (dblock_t*)create_block();

#if 0 // this version is a shallow copy of the heavy data items, but more verbose programming

            // copy values from SerBlock b* to diy block d*
            d->gid                  = b[i].gid;
            d->bounds               = b[i].bounds;
            d->box                  = b[i].box;
            d->data_bounds          = b[i].data_bounds;
            d->num_orig_particles   = b[i].num_orig_particles;
            d->num_particles        = b[i].num_particles;
            d->particles            = b[i].particles;          // shallow copy, pointer only
            d->rem_gids             = b[i].rem_gids;           // shallow
            d->rem_lids             = b[i].rem_lids;           // shallow
            d->complete             = b[i].complete;
            d->num_tets             = b[i].num_tets;
            d->tets                 = b[i].tets;               // shallow
            d->vert_to_tet          = b[i].vert_to_tet;        // shallow
            d->num_grid_pts         = 0;
            d->density              = NULL;

#else  // or this version is a deep copy of all items, but easier to program

            // copy serialized buffer to diy block
            diy::MemoryBuffer bb;
            //bb.buffer.resize(b[i].diy_bb.size());
            //swap(b[i].diy_bb, bb.buffer);
            //copy(b[i].diy_bb.begin(), b[i].diy_bb.end(), bb.buffer.begin());
            bb.buffer = vector<char>(b[i].diy_bb);

            load_block_light(d, bb);
            fprintf(stderr, "Loading %i particles\n", d->num_orig_particles );
            fprintf(stderr, "Loading %i with ghost particles\n", d->num_particles );

#endif

            // copy link
            diy::MemoryBuffer lb;
            //lb.buffer.resize(b[i].diy_lb.size());
            //swap(b[i].diy_lb, lb.buffer);
            //copy(b[i].diy_lb.begin(), b[i].diy_lb.end(), lb.buffer.begin());
            lb.buffer = vector<char>(b[i].diy_lb);
            diy::Link* link = diy::LinkFactory::load(lb);
            link->fix(assigner);



            // add the block to the master
            master.add(d->gid, d, link);

            // debug
            // fprintf(stderr, "dense: lid=%d gid=%d\n", i, d->gid);
            // fprintf(stderr, "mins [%.3f %.3f %.3f] maxs [%.3f %.3f %.3f]\n",
            //         d->bounds.min[0], d->bounds.min[1], d->bounds.min[2],
            //         d->bounds.max[0], d->bounds.max[1], d->bounds.max[2]);
            // fprintf(stderr, "box min [%.3f %.3f %.3f] max [%.3f %.3f %.3f]\n",
            //         d->box.min[0], d->box.min[1], d->box.min[2],
            //         d->box.max[0], d->box.max[1], d->box.max[2]);
            // fprintf(stderr, "data_bounds min [%.3f %.3f %.3f] max [%.3f %.3f %.3f]\n",
            //         d->data_bounds.min[0], d->data_bounds.min[1], d->data_bounds.min[2],
            //         d->data_bounds.max[0], d->data_bounds.max[1], d->data_bounds.max[2]);
            // fprintf(stderr, "num_orig_particles %d num_particles %d\n",
            //         d->num_orig_particles, d->num_particles);
            // fprintf(stderr, "complete %d num_tets %d\n", d->complete, d->num_tets);

            // fprintf(stderr, "particles:\n");
            // for (int j = 0; j < d->num_particles; j++)
            //     fprintf(stderr, "gid=%d j=%d [%.3f %.3f %.3f] ", d->gid, j,
            //             d->particles[3 * j], d->particles[3 * j + 1], d->particles[3 * j + 2]);
            // fprintf(stderr, "\n");

            // fprintf(stderr, "rem_gids:\n");
            // for (int j = 0; j < d->num_particles - d->num_orig_particles; j++)
            //     fprintf(stderr, "gid=%d j=%d rem_gid=%d ", d->gid, j, d->rem_gids[j]);
            // fprintf(stderr, "\n");

            // fprintf(stderr, "rem_lids:\n");
            // for (int j = 0; j < d->num_particles - d->num_orig_particles; j++)
            //     fprintf(stderr, "gid=%d j=%d rem_lid=%d ", d->gid, j, d->rem_lids[j]);
            // fprintf(stderr, "\n");

            // fprintf(stderr, "tets:\n");
            // for (int j = 0; j < d->num_tets; j++)
            //     fprintf(stderr, "gid=%d tet[%d]=[%d %d %d %d; %d %d %d %d] ",
            //             d->gid, j,
            //             d->tets[j].verts[0], d->tets[j].verts[1],
            //             d->tets[j].verts[2], d->tets[j].verts[3],
            //             d->tets[j].tets[0], d->tets[j].tets[1],
            //             d->tets[j].tets[2], d->tets[j].tets[3]);
            // fprintf(stderr, "\n");

            // fprintf(stderr, "vert_to_tet:\n");
            // for (int j = 0; j < d->num_particles; j++)
            //     fprintf(stderr, "gid=%d vert_to_tet[%d]=%d ", d->gid, j, d->vert_to_tet[j]);
            // fprintf(stderr, "\n");
        }
    } else
        fprintf(stderr, "Error: null pointer for array of blocks in dense\n");
}

// consumer
void density_estimate(
        Decaf*      decaf,
        int         tot_blocks,
        string      outfile_base,
        MPI_Comm    comm)
{
    // set some default arguments
    float mass           = 1.0;                 // particle mass
    alg alg_type         = DENSE_TESS;          // tess or cic
    int num_given_bounds = 0;                   // number of given bounds
    bool project         = true;                // whether to project to 2D
    float proj_plane[3]  = {0.0, 0.0, 1.0};     // normal to projection plane
    int glo_num_idx[3]   = {512, 512, 512};     // global grid number of points
    int num_threads      = 1;                   // threads diy can use
    int mem_blocks       = -1;                  // max blocks in memory
    char full_outfile[256];                     // full output file name

    int nblocks;                                // my local number of blocks
    int maxblocks;                              // max blocks in any process
    dblock_t *dblocks;                          // delaunay local blocks
    float eps = 0.0001;                         // epsilon for floating point values to be equal
    float data_mins[3], data_maxs[3];           // data global bounds

    // grid bounds
    float given_mins[3], given_maxs[3];         // the given bounds
    float grid_phys_mins[3], grid_phys_maxs[3]; // grid physical bounds
    float grid_step_size[3];                    // physical size of one grid space

    // ensure projection plane normal vector is unit length
    float length = sqrt(proj_plane[0] * proj_plane[0] +
                        proj_plane[1] * proj_plane[1] +
                        proj_plane[2] * proj_plane[2]);
    proj_plane[0] /= length;
    proj_plane[1] /= length;
    proj_plane[2] /= length;

    // event loop
    // TODO: verify that all of the below can run iteratively, only tested for one time step
    static int step = 0;

    vector<pConstructData> in_data;
    while (decaf->get(in_data))
    {
        fprintf(stderr, "dense: getting data for step %d\n", step);

        // timing
        double times[DENSE_MAX_TIMES];               // timing
        MPI_Barrier(comm);
        times[TOTAL_TIME] = MPI_Wtime();
        times[INPUT_TIME] = MPI_Wtime();

        MPI_Barrier(comm);
        times[INPUT_TIME] = MPI_Wtime() - times[INPUT_TIME];
        times[COMP_TIME] = MPI_Wtime();

        fprintf(stderr, "Size of my producer on inbound 0: %i\n", decaf->prod_comm_size(0));

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
        // debug: for reading file
        //diy::RoundRobinAssigner   assigner(world.size(), -1);  // tot_blocks found by read_blocks

        //diy::RoundRobinAssigner   assigner(world.size(), tot_blocks);
        diy::ContiguousAssigner   assigner(world.size(), tot_blocks);

        // fill blocks with incoming data
        fill_blocks(in_data, master, assigner);


        // debug: read the tessellation
        //diy::io::read_blocks("del-4-2.out", world, assigner, master, &load_block_light);
        //tot_blocks = assigner.nblocks();

        // debug: displaying the content of the block
        //master.foreach(&output);


        // get global block quantities
        nblocks = master.size();                    // local number of blocks
        MPI_Allreduce(&nblocks, &maxblocks, 1, MPI_INT, MPI_MAX, comm);
        MPI_Allreduce(&nblocks, &tot_blocks, 1, MPI_INT, MPI_SUM, comm);
        fprintf(stderr, "max_blocks per proc: %i, total_blocks: %i\n", maxblocks, tot_blocks);

        // compute the density
        dense(alg_type, num_given_bounds, given_mins, given_maxs, project, proj_plane,
              mass, data_mins, data_maxs, grid_phys_mins, grid_phys_maxs, grid_step_size, eps,
              glo_num_idx, master);

        MPI_Barrier(comm);
        times[COMP_TIME] = MPI_Wtime() - times[COMP_TIME];

        // write file
        // NB: all blocks need to be in memory; WriteGrid is not diy2'ed yet
        times[OUTPUT_TIME] = MPI_Wtime();
        sprintf(full_outfile, "%s%d.raw", outfile_base.c_str(), step);
        WriteGrid(maxblocks, tot_blocks, full_outfile, project, glo_num_idx, eps, data_mins, data_maxs,
                  num_given_bounds, given_mins, given_maxs, master, assigner);
        MPI_Barrier(comm);
        times[OUTPUT_TIME] = MPI_Wtime() - times[OUTPUT_TIME];


        MPI_Barrier(comm);
        times[TOTAL_TIME] = MPI_Wtime() - times[TOTAL_TIME];

        dense_stats(times, master, grid_step_size, grid_phys_mins, glo_num_idx);

        step++;
    } // decaf event loop

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "density estimation terminating\n");
    decaf->terminate();
}

int main(int argc,
         char** argv)
{
    MPI_Init(NULL, NULL);

    if(argc < 5)
    {
        fprintf(stderr, "Usage: dense <unused> <unused> tot_blocks outfile\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    int tot_blocks = atoi(argv[3]);
    string outfile = argv[4];
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
    density_estimate(decaf, tot_blocks, outfile, decaf->con_comm_handle());

    // timing
    MPI_Barrier(decaf->con_comm_handle());
    if (decaf->con_comm()->rank() == 0)
    {
        fprintf(stderr, "*** dense elapsed time = %.3lf s. ***\n", MPI_Wtime() - t0);
        fprintf(stderr, "*** workflow stopping time = %.3lf s. ***\n", MPI_Wtime());
    }

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished dense\n");
    return 0;
}
