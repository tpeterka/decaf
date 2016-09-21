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
#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

#include "wflow.hpp"                         // defines the workflow for this example
#include "tess/tess.h"
#include "tess/tess.hpp"
#include "tess/dense.hpp"
#include "block_serialization.hpp"

// using namespace decaf;
using namespace std;

// fill blocks with incoming data
// TODO: for now just prints some of the data; need to add to blocks in the master
void fill_blocks(vector<pConstructData>& in_data)
{
    // TOOD: get blocks as input args and fill them with the in_data
    // for now just printing it

    // only using first message in in_data (in_data[0])
    ArrayField<dblock_t> af = in_data[0]->getFieldData<ArrayField<dblock_t> >("blocks_array");
    if (af)
    {
        dblock_t* b = af.getArray();

        for (int i = 0; i < af.getNbItems(); i++)
        {
            fprintf(stderr, "block gid = %d\n", b[i].gid);

            // TODO: print the rest of the fields
        }
        // TODO: add the blocks to the master
    } else
        fprintf(stderr, "Error: null pointer for array of blocks in dense\n");
}

// consumer
void density_estimate(Decaf* decaf, MPI_Comm comm)
{
    // this first test just does a density estimation of a tessellation read from a file

    // set some default arguments
    int tot_blocks       = 2;                   // global number of blocks
    float mass           = 1.0;                 // particle mass
    alg alg_type         = DENSE_TESS;          // tess or cic
    int num_given_bounds = 0;                   // number of given bounds
    bool project         = true;                // whether to project to 2D
    float proj_plane[3]  = {0.0, 0.0, 1.0};     // normal to projection plane
    int glo_num_idx[3]   = {512, 512, 512};     // global grid number of points
    int num_threads      = 1;                   // threads diy can use
    int mem_blocks       = -1;                  // max blocks in memory
    char infile[256]     = "del.out";           // input file name
    char outfile[256]    = "dense.raw";         // output file name

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
    vector<pConstructData> in_data;
    while (decaf->get(in_data))
    {
        // timing
        double times[DENSE_MAX_TIMES];               // timing
        MPI_Barrier(comm);
        times[TOTAL_TIME] = MPI_Wtime();
        times[INPUT_TIME] = MPI_Wtime();

        MPI_Barrier(comm);
        times[INPUT_TIME] = MPI_Wtime() - times[INPUT_TIME];
        times[COMP_TIME] = MPI_Wtime();

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
        diy::RoundRobinAssigner   assigner(world.size(), -1);  // tot_blocks found by read_blocks

        // fill blocks with incoming data
        fill_blocks(in_data);

        // read the tessellation
        diy::io::read_blocks(infile, world, assigner, master, &load_block_light);
        tot_blocks = assigner.nblocks();

        // get global block quantities
        nblocks = master.size();                    // local number of blocks
        MPI_Allreduce(&nblocks, &maxblocks, 1, MPI_INT, MPI_MAX, comm);
        MPI_Allreduce(&nblocks, &tot_blocks, 1, MPI_INT, MPI_SUM, comm);

        // compute the density
        dense(alg_type, num_given_bounds, given_mins, given_maxs, project, proj_plane,
              mass, data_mins, data_maxs, grid_phys_mins, grid_phys_maxs, grid_step_size, eps,
              glo_num_idx, master);

        MPI_Barrier(comm);
        times[COMP_TIME] = MPI_Wtime() - times[COMP_TIME];

        // write file
        // NB: all blocks need to be in memory; WriteGrid is not diy2'ed yet
        times[OUTPUT_TIME] = MPI_Wtime();
        WriteGrid(maxblocks, tot_blocks, outfile, project, glo_num_idx, eps, data_mins, data_maxs,
                  num_given_bounds, given_mins, given_maxs, master, assigner);
        MPI_Barrier(comm);
        times[OUTPUT_TIME] = MPI_Wtime() - times[OUTPUT_TIME];


        MPI_Barrier(comm);
        times[TOTAL_TIME] = MPI_Wtime() - times[TOTAL_TIME];

        dense_stats(times, master, grid_step_size, grid_phys_mins, glo_num_idx);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "density estimation terminating\n");
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
    density_estimate(decaf, decaf->con_comm_handle());

    // cleanup
    delete decaf;
    MPI_Finalize();

    fprintf(stderr, "finished dense\n");
    return 0;
}
