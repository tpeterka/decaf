//---------------------------------------------------------------------------
//
// example of creating datatypes
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <mpi.h>

// one tetrahedron
// copied from https://bitbucket.org/diatomic/tess/include/tess/tet.h
struct tet_t
{
  int verts[4];		// indices of the vertices
  int tets[4];		// indices of the neighbors
			// tets[i] lies opposite verts[i]
};
// delaunay tessellation for one block
// copied from https://bitbucket.org/diatomic/tess/include/tess/delaunay.h
struct dblock_t
{
  int gid;                // global block id
  float mins[3];          // block min extent
  float maxs[3];          // block max extent
  // input particles
  int num_orig_particles; // number of original particles in this block before exchanges
  int num_particles;      // current number of particles in this block after any exchanges
  float *particles;       // original input points
  // tets
  int num_tets;           // number of delaunay tetrahedra
  struct tet_t *tets;     // delaunay tets
  int *rem_gids;          // owners of remote particles
  int* vert_to_tet;       // a tet that contains the vertex
};

using namespace decaf;

// user-defined selector code
// runs in the producer
// selects from decaf->data() the subset going to dataflow
// sets that selection in decaf->data()
// returns number of items selected
// items must be in terms of the elementary datatype defined in decaf->data()
int selector(Decaf* decaf)
{
  // this example is simply passing through the original single integer
  // decaf->data()->data_ptr remains unchanged, and the number of datatypes remains 1
  return 1;
}
// user-defined pipeliner code
void pipeliner(Decaf* decaf)
{
}
// user-defined resilience code
void checker(Decaf* decaf)
{
}
//
// gets command line args
//
void GetArgs(int argc, char **argv, DecafSizes& decaf_sizes, int& prod_nsteps)
{
  assert(argc >= 9);

  decaf_sizes.prod_size    = atoi(argv[1]);
  decaf_sizes.dflow_size   = atoi(argv[2]);
  decaf_sizes.con_size     = atoi(argv[3]);

  decaf_sizes.prod_start   = atoi(argv[4]);
  decaf_sizes.dflow_start  = atoi(argv[5]);
  decaf_sizes.con_start    = atoi(argv[6]);

  prod_nsteps              = atoi(argv[7]); // user's, not decaf's variable
  decaf_sizes.con_nsteps   = atoi(argv[8]);
}

void run(DecafSizes& decaf_sizes, int prod_nsteps)
{
  MPI_Init(NULL, NULL);

  // create some data types

  // 100 3d points
  VectorDatatype* vec_type = new VectorDatatype(128 * 3, 1, MPI_FLOAT);

  // 30 x 100 density values
  float density[30][100];
  int full_size[] = {100, 30}; // always [x][y]... order (not C order!)
  int sub_size[]  = {2, 30};   // always [x][y]... order
  int start_pos[] = {0, 0};    // always [x][y]... order
  SliceDatatype* slice_type = new SliceDatatype(2, full_size, sub_size, start_pos, MPI_FLOAT);

  // datatype for tet
  DataElement tet_map[] =
  {
    { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, verts) },
    { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, tets)  },
  };
  StructDatatype* tet_type = new StructDatatype(0, sizeof(tet_map) / sizeof(tet_map[0]), tet_map);

  dblock_t d; // delaunay block (TODO: needs to be initialized)

  // datatype for delaunay block
  int num_rem_particles = d.num_particles - d.num_orig_particles;
  MPI_Datatype* dtype = tet_type->comm_datatype();
  DataElement del_map[] =
  {
    { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, gid)                },
    { MPI_FLOAT, DECAF_OFST, 3,                   offsetof(struct dblock_t, mins)               },
    { MPI_FLOAT, DECAF_OFST, 3,                   offsetof(struct dblock_t, maxs)               },
    { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_orig_particles) },
    { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_particles)      },
    { MPI_FLOAT, DECAF_ADDR, d.num_particles * 3, addressof(d.particles)                        },
    { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_tets)           },
    { *dtype,    DECAF_ADDR, d.num_tets,          addressof(d.tets)                             },
    { MPI_INT,   DECAF_ADDR, num_rem_particles,   addressof(d.rem_gids)                         },
    { MPI_INT,   DECAF_ADDR, d.num_particles,     addressof(d.vert_to_tet)                      },
  };
  StructDatatype* del_type = new StructDatatype(0, sizeof(del_map) / sizeof(del_map[0]), del_map);

  // cleanup the datatypes created above
  delete vec_type;
  delete slice_type;
  delete tet_type;
  delete del_type;

  // the rest of this example is the same as direct.cpp
  // TODO: use the datatypes created above in the dataflow

  // define the data type
  Data data(MPI_INT);

  // start decaf, allocate on the heap instead of on the stack so that it can be deleted
  // before MPI_Finalize is called at the end
  Decaf* decaf = new Decaf(MPI_COMM_WORLD,
                           decaf_sizes,
                           &selector,
                           &pipeliner,
                           &checker,
                           &data);
  decaf->err();

  // producer and consumer data in separate pointers in case producer and consumer overlap
  int *pd, *cd;
  int con_interval = prod_nsteps / decaf_sizes.con_nsteps; // consume every so often

  for (int t = 0; t < prod_nsteps; t++)
  {
    // producer
    if (decaf->is_prod())
    {
      pd = new int[1];
      // any custom producer (eg. simulation code) goes here or gets called from here
      // as long as put() gets called at that desired frequency
      *pd = t;
      fprintf(stderr, "+ producing time step %d, val %d\n", t, *pd);
      // assumes the consumer has the previous value, ok to overwrite
      // check your modulo arithmetic to ensure you put exactly decaf->con_nsteps times
      if (!((t + 1) % con_interval))
        decaf->put(pd);
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly decaf->con_nsteps times
    if (decaf->is_con() && !((t + 1) % con_interval))
    {
      // any custom consumer (eg. data analysis code) goes here or gets called from here
      // as long as get() gets called at that desired frequency
      cd = (int*)decaf->get();
      // for example, add all the items arrived at this rank
      int sum = 0;
      fprintf(stderr, "consumer get_nitems = %d\n", decaf->get_nitems());
      for (int i = 0; i < decaf->get_nitems(); i++)
        sum += cd[i];
      fprintf(stderr, "- consuming time step %d, sum = %d\n", t, sum);
    }

    decaf->flush(); // both producer and consumer need to clean up after each time step
    // now safe to cleanup producer data, after decaf->flush() is called
    // don't wory about deleting the data pointed to by cd; decaf did that in fluwh()
    if (decaf->is_prod())
      delete[] pd;
  }

  // cleanup
  delete decaf;
  MPI_Finalize();
}

int main(int argc, char** argv)
{
  // parse command line args
  DecafSizes decaf_sizes;
  int prod_nsteps;
  GetArgs(argc, argv, decaf_sizes, prod_nsteps);

  // run decaf
  run(decaf_sizes, prod_nsteps);

  return 0;
}
