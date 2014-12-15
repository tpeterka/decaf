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

// delaunay block datatype function generator
void create_delaunay_datatype(const struct dblock_t* d, int* map_count, DataElement** map, MPI_Datatype* mpi_map){
  
	// tet data map
	DataElement tet_map[] =
  {
    { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, verts) },
    { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, tets)  },
  };

  StructDatatype* tet_type = new StructDatatype(0, sizeof(tet_map) / sizeof(tet_map[0]), tet_map);
  MPI_Datatype* ttype = tet_type->comm_datatype();
  DataElement del_map[] =
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
  if(mpi_map){
  	StructDatatype* del_type = new StructDatatype((MPI_Aint) d, sizeof(del_map) / sizeof(del_map[0]), del_map);
		*mpi_map = *(del_type->comm_datatype());
	}

	// save the map if needed
	if(map_count){
	  *map_count = sizeof(del_map)/sizeof(del_map[0]);
		*map = new DataElement[*map_count]();
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
void GetArgs(int argc, char **argv, DecafSizes& decaf_sizes)
{
  assert(argc >= 9);

  decaf_sizes.prod_size    = atoi(argv[1]);
  decaf_sizes.dflow_size   = atoi(argv[2]);
  decaf_sizes.con_size     = atoi(argv[3]);

  decaf_sizes.prod_start   = atoi(argv[4]);
  decaf_sizes.dflow_start  = atoi(argv[5]);
  decaf_sizes.con_start    = atoi(argv[6]);

  decaf_sizes.prod_nsteps  = atoi(argv[7]);
  decaf_sizes.con_interval = atoi(argv[8]);
}

void run(DecafSizes& decaf_sizes)
{
  MPI_Init(NULL, NULL);

  // create some data types

  // 100 3d points
  //VectorDatatype* vec_type = new VectorDatatype(128 * 3, 1, MPI_FLOAT);

  // 30 x 100 density values
  //float density[30][100];
  //int full_size[] = {100, 30}; // always [x][y]... order (not C order!)
  //int sub_size[]  = {2, 30};   // always [x][y]... order
  //int start_pos[] = {0, 0};    // always [x][y]... order
  //SliceDatatype* slice_type = new SliceDatatype(2, full_size, sub_size, start_pos, MPI_FLOAT);

  // datatype for tet
  //DataElement tet_map[] =
  //{
  //  { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, verts) },
  //  { MPI_INT, DECAF_OFST, 4, offsetof(struct tet_t, tets)  },
  //};
  //StructDatatype* tet_type = new StructDatatype(0, sizeof(tet_map) / sizeof(tet_map[0]), tet_map);
  //int debugWait = 1;
	//while(debugWait);
	float* particles= new float[90];
	struct tet_t* tets = new struct tet_t[7];
	tets[0].verts[2] = 5;
	int* rem_gids = new int[5];
	int* vert_to_tet = new int[30];
  dblock_t d = 
  	  { 100, {0,0,0}, {10,10,10}, 25, 30, particles, 7, tets, 
		  rem_gids, vert_to_tet,
		}; // delaunay block (TODO: needs to be initialized)
	//  { 100, {0,0,0} 
	//	}; // delaunay block (TODO: needs to be initialized)
  //dblock_t* dp = new dblock_t();
	//memcpy(dp, &d, sizeof(dblock_t));
	//d.num_particles = 20;
	//d.num_orig_particles = 15;
	//d.num_tets = 10;
	// fprintf(stdout, "dblock created: {%d, [%f,%f,%f], [%f,%f,%f], %d, %d, *[%f,..,%f], %d, *[{%d,%d},..., {%d,%d}], *[%d,...,%d], *[%d,...,%d]}\n",
	//        d.gid, d.mins[0], d.mins[1], d.mins[2], d.maxs[0], d.maxs[1], d.maxs[2],
	// 				d.num_orig_particles, d.num_particles, d.particles[0], d.particles[3*d.num_particles-1], d.num_tets, 
	// 				d.tets[0].verts[0], d.tets[0].tets[0], d.tets[d.num_tets-1].verts[3], d.tets[d.num_tets-1].tets[3], d.rem_gids[0], d.rem_gids[d.num_particles-d.num_orig_particles-1], 
	// 				d.vert_to_tet[0], d.vert_to_tet[d.num_particles-1]);

  // Data declaration
	DataBis<dblock_t> delaunayData(create_delaunay_datatype);
 
  // datatype for delaunay block
	//int num_rem_particles = d.num_particles - d.num_orig_particles;
  //MPI_Datatype* dtype = tet_type->comm_datatype();
  //DataElement del_map[] =
  //{
  //  { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, gid)                },
  //  { MPI_FLOAT, DECAF_OFST, 3,                   offsetof(struct dblock_t, mins)               },
  //  { MPI_FLOAT, DECAF_OFST, 3,                   offsetof(struct dblock_t, maxs)               },
  //  { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_orig_particles) },
  //  { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_particles)      },
  //  { MPI_FLOAT, DECAF_ADDR, d.num_particles * 3, addressof(d.particles)                        },
  //  //{ MPI_FLOAT, DECAF_ADDR, 20 * 3, addressof(d.particles)                        },
  //  { MPI_INT,   DECAF_OFST, 1,                   offsetof(struct dblock_t, num_tets)           },
  //  { *dtype,    DECAF_ADDR, d.num_tets,          addressof(d.tets)                             },
  //  //{ *dtype,    DECAF_ADDR, 10,          addressof(d.tets)                             },
  //  { MPI_INT,   DECAF_ADDR, d.num_particles-d.num_orig_particles, addressof(d.rem_gids)                         },
  //  //{ MPI_INT,   DECAF_ADDR, 5,   addressof(d.rem_gids)                         },
  //  { MPI_INT,   DECAF_ADDR, d.num_particles,   addressof(d.vert_to_tet)                      },
  //  //{ MPI_INT,   DECAF_ADDR, 40,     addressof(d.vert_to_tet)                      },
  //};
  //StructDatatype* del_type = new StructDatatype((MPI_Aint) &d, sizeof(del_map) / sizeof(del_map[0]), del_map);
	
	// Create a data & MPI Map for var d
	MPI_Datatype del_mpi_map;
	int map_count = 0;
	DataElement* map;
	create_delaunay_datatype(&d, &map_count, &map, &del_mpi_map);
	
	// Check if the map is correctly set
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	for (int i=0; i<map_count; i++ )
     if (rank == 0)
       fprintf(stdout, "[%d] ***Processing DataElement %d: (%p, %p, %d, %p)\n", rank,
               i, map[i].base_type, map[i].disp_type, map[i].count, map[i].disp);
	
	// get an MPI Map from a data Map
	MPI_Datatype del_mpi_map_bis;
	//const DataElement* map_const = map;
	delaunayData.getMPIDatatypeFromMap(map_count, map, &del_mpi_map_bis);
  
	// split test
	MPI_Datatype chunk_mpi_map;
	if(rank == 0 || rank == 1){
	  vector<vector<DataElement*> > maps;
		maps = delaunayData.split(map_count, map, 4);	
	  vector<DataElement*> chunk_vector = maps[1];
		int chunk_map_count = chunk_vector.size();
		DataElement chunk_map[chunk_map_count];
		for(int i=0; i<chunk_map_count; i++){
		  if(rank == 0) 
			  fprintf(stdout, "[%d] '''Processing Element %d: (%p, %p, %d, %p)\n", rank, i, 
				        chunk_vector[i]->base_type, chunk_vector[i]->disp_type, chunk_vector[i]->count,
								chunk_vector[i]->disp);
		  memcpy(&chunk_map[i], &chunk_vector[i], sizeof(DataElement));	
		}
		delaunayData.getMPIDatatypeFromMap(chunk_map_count, chunk_map, &chunk_mpi_map);
	}
	//create_delaunay_datatype(&d, NULL, NULL, &del_mpi_map);
	//MPI_Datatype del_map_mpi;
  //StructDatatype* del_type = new StructDatatype(0, sizeof(del_map) / sizeof(del_map[0]), del_map);
	//if(!rank) del_type->split(4);
	//int del_type_size = 0;
	//MPI_Aint extent = 0;
  //MPI_Type_size(*(del_type->comm_datatype()), &del_type_size);	
  //MPI_Type_extent(*(del_type->comm_datatype()), &extent);	
	//fprintf(stdout, "size of the type del_type = %d - extent %ld \n", del_type_size, extent);
	//fprintf(stdout, "size of the type del_type = %d - %p - with MPI_Address %p - and the real addr %p\n", del_type_size, d.tets, addressof(d.tets), tets);
 
  //MPI_Datatype _del_map;
	//vector<int>  _counts;
	//vector<MPI_Aint> _addrs;
	//vector<MPI_Datatype> _base_types;
  //
	//_addrs.push_back(offsetof(struct dblock_t, gid));
	//_counts.push_back(1);
	//_base_types.push_back(MPI_INT);
  // 
	//_addrs.push_back(offsetof(struct dblock_t, mins));
	//_counts.push_back(3);
	//_base_types.push_back(MPI_FLOAT);
  //
	//MPI_Type_create_struct(_base_types.size(), &_counts[0], &_addrs[0], &_base_types[0], &_del_map);
  //MPI_Datatype _del_map;
	//int  _counts[2];
	//MPI_Aint _addrs[2];
	//MPI_Datatype _base_types[2];

	//_addrs[0] = offsetof(struct dblock_t, gid);
	//_counts[0] = 1;
	//_base_types[0] = MPI_INT;
 
	//_addrs[1] = offsetof(struct dblock_t, mins);
	//_counts[1] = 3;
	//_base_types[1] = MPI_FLOAT;

	//MPI_Type_create_struct(2, _counts, _addrs, _base_types, &_del_map);
	//MPI_Type_commit(&_del_map);

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
		d.gid = 103;
		d.particles[26] = 999;
		d.particles[28] = 999;
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
	 fprintf(stdout, "DBlock received with id %d - and particles[26]=%f particles[27]=%f\n", d.gid, d.particles[26], d.particles[27]);

	 //struct dblock_t* d_new = new dblock_t();
	 //struct tet_t* t_new = new tet_t();
	 //float* particles= new float[90];
	 //struct tet_t* tets = new struct tet_t[7];
	 //tets[0].verts[2] = 5;
	 //int* rem_gids = new int[5];
	 //int* vert_to_tet = new int[30];
   //dblock_t d_new = 
   //	  { 0, {0,0,0}, {0,0,0}, 0, 0, particles, 0, tets, 
	 //	  rem_gids, vert_to_tet,
	 //	}; // delaunay block (TODO: needs to be initialized)
	 //MPI_Recv(&rank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	 //fprintf(stdout, "Received %d\n", rank);
	 //MPI_Recv(t_new, 1, *(tet_type->comm_datatype()), MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	 //MPI_Recv(MPI_BOTTOM, 1, *(del_type->comm_datatype()), MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	 //MPI_Recv(d_new, 1, del_map_mpi, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	 //fprintf(stdout, "{[%d, %d, %d, %d],[%d, %d, %d, %d]}\n", t_new->verts[0], t_new->verts[1], t_new->verts[2], t_new->verts[3],
	 //                t_new->tets[0], t_new->tets[1], t_new->tets[2], t_new->tets[3]);
	 //fprintf(stdout, "dblock received: {%d, [%f,%f,%f], [%f,%f,%f], %d, %d, %p, %d, {%d,%d}, [%d,...,%d], [%d,...,%d]}\n",
	 //       d_new->gid, d_new->mins[0], d_new->mins[1], d_new->mins[2], d_new->maxs[0], d_new->maxs[1], d_new->maxs[2],
	 //				d_new->num_orig_particles, d_new->num_particles, d_new->particles, d_new->num_tets, 
	 //				d_new->tets[0].verts[0], d_new->tets[0].tets[0], d_new->rem_gids[0], d_new->rem_gids[1], 
	 //				d_new->vert_to_tet[0], d_new->vert_to_tet[1]);
	 //fprintf(stdout, "Data received in %d", d.gid);
  }
  // cleanup the datatypes created above
  //delete vec_type;
  //delete slice_type;
  //delete tet_type;
  //delete del_type;

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

  // producer and consumer data
  // keep these in separate pointers in ase producer and consumer overlap
  int *pd, *cd;

  for (int t = 0; t < decaf_sizes.prod_nsteps; t++)
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
      if (!(t % decaf_sizes.con_interval))
        decaf->put(pd);
    }

    // consumer
    if (decaf->is_con() && !(t % decaf_sizes.con_interval))
    {
      // any custom consumer (eg. data analysis code) goes here or gets called from here
      // as long as get() gets called at that desired frequency
      cd = (int*)decaf->get();
      // for example, add all the items arrived at this rank
      int sum = 0;
      fprintf(stderr, "consumer get_nitems = %d\n", decaf->data()->get_nitems());
      for (int i = 0; i < decaf->data()->get_nitems(); i++)
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
  GetArgs(argc, argv, decaf_sizes);

  // run decaf
  run(decaf_sizes);

  return 0;
}
