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
#include <stdint.h>
#include <mpi.h>

#include <mpi_debug.c>
#define MPI_DEBUG 0

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

// user-defined
// delaunay block datatype function generator
void create_delaunay_datatype(const struct dblock_t* d, int* map_count, DataMap** map, CommDatatype* comm_datatype){

  // tet data map
  DataMap tet_map[] =
    {
      { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, verts) },
      { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, tets)  },
    };

  StructDatatype* tet_type = new StructDatatype(0, sizeof(tet_map) / sizeof(tet_map[0]), tet_map);
  MPI_Datatype* ttype = tet_type->comm_datatype();
  DataMap del_map[] =
    {
      { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, gid)                },
      { MPI_FLOAT, DECAF_OFST, 3,                   offsetof(struct dblock_t, mins)               },
      { MPI_FLOAT, DECAF_OFST, 3,                   offsetof(struct dblock_t, maxs)               },
      { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_orig_particles) },
      { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_particles)      },
      { MPI_FLOAT, DECAF_ADDR, d->num_particles * 3, addressof(d->particles)                      },
      { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_tets)           },
      { *ttype,    DECAF_ADDR, d->num_tets,          addressof(d->tets)                           },
      { MPI_INT,   DECAF_ADDR, d->num_particles-d->num_orig_particles, addressof(d->rem_gids)      },
      { MPI_INT,   DECAF_ADDR, d->num_particles,   addressof(d->vert_to_tet)                      },
    };

  // generate MPI datatype if needed
  if(comm_datatype){
    StructDatatype* del_type = new StructDatatype((MPI_Aint) d, sizeof(del_map) / sizeof(del_map[0]), del_map);
    *comm_datatype = *(del_type->comm_datatype());
  }

  // save the map if needed
  if(map_count){
    *map_count = sizeof(del_map)/sizeof(del_map[0]);
    *map = new DataMap[*map_count]();
    memcpy(*map, del_map, sizeof(del_map));
  }

}

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

  float* particles= new float[90];
  struct tet_t* tets = new struct tet_t[7];
  tets[0].verts[2] = 5;
  int* rem_gids = new int[5];
  int* vert_to_tet = new int[30];
  dblock_t d =
    { 100, {0,0,0}, {10,10,10}, 25, 30, particles, 7, tets,
      rem_gids, vert_to_tet,
    };

  // Data declaration
  DataBis<dblock_t> delaunayData(create_delaunay_datatype);


  // Create a data & MPI Map for var d
  MPI_Datatype del_mpi_map;
  int map_count = 0;
  DataMap* map;
  create_delaunay_datatype(&d, &map_count, &map, &del_mpi_map);

  // Check if the map is correctly set
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  for (int i=0; i<map_count; i++ )
    if (rank == 0)
      fprintf(stdout, "[%d] ***Processing DataMap %d: (%p, %p, %d, %p)\n", rank,
              i, map[i].base_type, map[i].disp_type, map[i].count, map[i].disp);

  // get an MPI Map from a data Map
  MPI_Datatype del_mpi_map_bis;
  //const DataMap* map_const = map;
  delaunayData.getCommDatatypeFromMap(map_count, map, &del_mpi_map_bis);

  // split test
  MPI_Datatype chunk_mpi_map;
  if(rank == 0 || rank == 1){
    vector<vector<DataMap*> > maps;
    maps = delaunayData.split(map_count, map, 4);
    //if (MPI_DEBUG) MPI_Debug_pause(rank, 10);
    vector<DataMap*> chunk_vector = maps[3];
    int chunk_map_count = chunk_vector.size();
    DataMap chunk_map[chunk_map_count];
    for(int i=0; i<chunk_map_count; i++){
      if(rank == 0)
        fprintf(stdout, "[%d] '''Processing Element %d: (%p, %p, %d, %p)\n", rank, i,
                chunk_vector[i]->base_type, chunk_vector[i]->disp_type, chunk_vector[i]->count,
                chunk_vector[i]->disp);
      memcpy(&chunk_map[i], chunk_vector[i], sizeof(DataMap));
    }
    if (MPI_DEBUG) MPI_Debug_pause(rank, 10);
    delaunayData.getCommDatatypeFromMap(chunk_map_count, chunk_map, &chunk_mpi_map);
  } else {
    if (MPI_DEBUG) MPI_Debug_pause(rank, 100);
  }

  if (rank == 1){
    //MPI_Send(&rank, 1, MPI_INT, 0, 0 , MPI_COMM_WORLD);

    // test the MPI Map created using the create_delaunay_datatype function
    d.gid = 102;
    MPI_Send(MPI_BOTTOM, 1, del_mpi_map, 0, 0, MPI_COMM_WORLD);

    // test the MPI Map created using getMPIDatatypeFromMap on the Map created by
    // create_delaunay_datatype when first creating the MPI Map
    d.gid = 103;
    MPI_Send(MPI_BOTTOM, 1, del_mpi_map_bis, 0, 0, MPI_COMM_WORLD);

    // test sending chunks maps after splitting Dblock map
    d.gid = 104;
    d.particles[26] = 999;
    d.particles[27] = 999;
    d.particles[62] = 999;
    d.particles[63] = 999;
    d.rem_gids[0] = 999;
    d.rem_gids[4] = 999;
    d.vert_to_tet[10] = 999;
    MPI_Send(MPI_BOTTOM, 1, chunk_mpi_map, 0, 0, MPI_COMM_WORLD);
    //MPI_Send(MPI_BOTTOM, 1, del_mpi_map_bis, 0, 0, MPI_COMM_WORLD);
    //MPI_Send(MPI_BOTTOM, 1, *(del_type->comm_datatype()), 0, 0, MPI_COMM_WORLD);
    //MPI_Send(tets, 1, *(tet_type->comm_datatype()), 0, 0, MPI_COMM_WORLD);
  }

  if (rank == 0){
    MPI_Status status;

    // first receive
    MPI_Recv(MPI_BOTTOM, 1, del_mpi_map, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    fprintf(stdout, "DBlock received with id %d\n", d.gid);

    // second receive
    MPI_Recv(MPI_BOTTOM, 1, del_mpi_map_bis, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    fprintf(stdout, "DBlock received with id %d\n", d.gid);

    // third receive
    MPI_Recv(MPI_BOTTOM, 1, chunk_mpi_map, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    fprintf(stdout, "DBlock received with id %d - and particles[26]=%f particles[27]=%f particles[62]=%f particles[63]=%f "
            "rem_gids[0]=%d, rem_gids[4]=%d vert_to_tet[10]=%d\n", d.gid, d.particles[26], d.particles[27],
            d.particles[62], d.particles[63], d.rem_gids[0], d.rem_gids[4], d.vert_to_tet[10]);

  }
  // cleanup the datatypes created above

  // the rest of this example is the same as direct.cpp
  // TODO: use the datatypes created above in the dataflow

  // define the data type
  //DataBis<dblock_t> delaunayData(create_delaunay_datatype);
  //fprintf(stdout, "a delaunay data is created %d\n", delaunayData.getNumberElements());

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
    // don't wory about deleting the data pointed to by cd; decaf did that in flush()
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
