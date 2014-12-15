//---------------------------------------------------------------------------
//
// decaf datatypes implementation in mpi transport layer
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_TRANSPORT_MPI_DATA_HPP
#define DECAF_TRANSPORT_MPI_DATA_HPP

#include "../../data.hpp"
#include <mpi.h>
#include <stdio.h>

//
// creates a vector datatype
//
// num_elems: number of elements in the vector
// stride: number of elements between start of each element (usually 1)
// base_type: data type of vector elements
//
decaf::
VectorDatatype::
VectorDatatype(int num_elems, int stride, CommDatatype base_type)
{
  MPI_Type_vector(num_elems, 1, stride, base_type, &datatype_);
  MPI_Type_commit(&datatype_);
  absolute_address = false;
}
//
// creates a subarray datatype
//
// dim: number of elements in full_size, start_pos, sub_size
// full_size: full sizes of the array ([x][y][z] (not C) order)
// start_pos: starting indices of the array ([x][y][z] (not C) order)
// sub_size: desired sizes of subarray ([x][y][z] (not C) order)
// base_type: data type of array elements
//
decaf::
SliceDatatype::
SliceDatatype(int dim, int* full_size, int* sub_size, int* start_pos, CommDatatype base_type)
{
  // fortran order below is not a bug: I always want [x][y][z] order
  MPI_Type_create_subarray(dim, full_size, sub_size, start_pos,
			   MPI_ORDER_FORTRAN, base_type, &datatype_);
  MPI_Type_commit(&datatype_);
  absolute_address = false;
}
//
// creates a structure datatype
//
// basse_addr: base address added to relative OFST displacements
// num_elems: number of data elements in the typemap
// map: typemap
//
decaf::
StructDatatype::
StructDatatype(Address base_addr, int num_elems, DataElement* map)
{
  // form the vectors needed to create the struct datatype
  //vector <MPI_Aint> addrs(num_elems, base_addr);
  vector<MPI_Aint> addrs;
  //vector <int> counts(num_elems, 0);
  vector<int> counts;
  //vector <MPI_Datatype> base_types(num_elems, 0);
  vector<MPI_Datatype> base_types;

  // traverse the map, skipping elements with 0 count, and filling the above vectors
  int nelems = 0;
  for (int i = 0; i < num_elems; i++)
  {
    if (map[i].count <= 0)
      continue;
    // debug
     int my_rank = -1;
     MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
     if (my_rank == 0)
       fprintf(stdout, "[%d] Processing DataElement %d: (%p, %p, %d, %p)\n", my_rank,
               i, map[i].base_type, map[i].disp_type, map[i].count, map[i].disp);
    if (map[i].disp_type == DECAF_OFST){
      //addrs[nelems] = addrs[i] + map[i].disp;
      addrs.push_back(base_addr+map[i].disp);
      map[i].disp += base_addr;
    }else{
      //addrs[nelems] = map[i].disp;
      addrs.push_back(map[i].disp);
    }
    //counts[nelems] = map[i].count;
    counts.push_back(map[i].count);
    //base_types[nelems] = map[i].base_type;
    base_types.push_back(map[i].base_type);
    nelems++;
  }
  for( int i=0; i <nelems ; i++){
  //  fprintf(stdout, "map %d = (%d, %ld, %p)\n", i, counts[i], addrs[i], base_types[i]);
  }
  // create the datatype
  MPI_Type_create_struct(counts.size(), &counts[0], &addrs[0], &base_types[0], &datatype_);
  //memcpy(&datatype_, del_map, sizeof(del_map));
  MPI_Type_commit(&datatype_);
  absolute_address = true;
  int del_type_size = 0;
  MPI_Type_size(datatype_, &del_type_size);
  //fprintf(stdout, "size of the type del_type = %d (from inside)\n", del_type_size);
  MPI_Aint* addrs_debug = &addrs[0]; 
  
  if (addrs.size() > 4)
  	fprintf(stdout, "gid=%d, mins[0]= %f, maxs[2]= %f,particles[0]=%f, tets[0].verts[0]=%d\n",
          *((int*)addrs[0]), ((float*)addrs[1])[0], ((float*)addrs[2])[0], ((float*)addrs[5])[0], -1 );

  // save the map for later datatype processing
  map_num_elems_ = num_elems;
  map_ = (DataElement*) malloc(sizeof(DataElement)*num_elems); // too bad, don't use malloc
  memcpy(map_, map, sizeof(DataElement)*num_elems);
}

// split an StructDatatype
std::vector <std::vector<DataElement*> >
decaf::
StructDatatype::
split(int n)
{
  // get the total size of the Struct
  int total_size = 0;
  for( int i=0; i<map_num_elems_ ; i++)
    total_size += map_[i].count;
  fprintf(stdout, "Total Struct typemap is %d\n", total_size);

 // chunk size
 int chunk_size = ceil((float)total_size/(float)n); // test if total size is actually > than n
 fprintf(stdout, "Chunk size is %d\n", chunk_size);

 std::vector<std::vector<DataElement*> > type_chunks_all;
 std::vector<DataElement*>* type_chunk;
 DataElement* type_element = new DataElement();
 //*type_element = {map_[0].base_type, map_[0].disp_type, map_[0].count, map_[0].disp};
 memcpy(type_element, &map_[0], sizeof(DataElement));// do not keep this 
 int counter = 0;
 type_chunk = new std::vector<DataElement*>(); // allocate the first datatype chunk
 fprintf(stdout, "Type chunk %p created.\n", type_chunk);
 // loop over the OriginalDataElement to create new one
 for( int i=0; i<map_num_elems_; i++) {
   int element_count = map_[i].count;
   // decide if this type elemenet wil be splitted
   while(counter + element_count > chunk_size){
     type_element = new DataElement();
     //*type_element = {map_[i].base_type, map_[i].disp_type, map_[i].count, map_[i].disp};
     memcpy(type_element, &map_[i], sizeof(DataElement));// do not keep this 
     type_element->count = chunk_size - counter; // set the right count
     // we finished  a type chunk
     fprintf(stdout, "New type element %p created with count %d\n", type_element, type_element->count); 
     fprintf(stdout, "New type element %p {%p, %p, %ld, %p}\n", 
             type_element, type_element->base_type, type_element->disp_type, type_element->count, type_element->disp);
     type_chunk->push_back(type_element);
     type_chunks_all.push_back(*type_chunk);
     type_chunk = new std::vector<DataElement*>();
     fprintf(stdout, "Type chunk %p created.\n", type_chunk); 
     counter = 0;
     element_count -= type_element->count;
   }

   if (element_count != 0 && element_count != map_[i].count){
     type_element = new DataElement();
     //*type_element = {map_[i].base_type, map_[i].disp_type, map_[i].count, map_[i].disp};
     memcpy(type_element, &map_[i], sizeof(DataElement));// do not keep this 
     type_element->count = element_count;
     fprintf(stdout, "New type element %p created with count %d\n", type_element, type_element->count);
     fprintf(stdout, "New type element %p {%p, %p, %ld, %p}\n", 
             type_element, type_element->base_type, type_element->disp_type, type_element->count, type_element->disp);
     counter += element_count;
     type_chunk->push_back(type_element);
     continue;
   }

   type_element = new DataElement();
   //*type_element = {map_[i].base_type, map_[i].disp_type, map_[i].count, map_[i].disp};
   memcpy(type_element, &map_[i], sizeof(DataElement));// do not keep this 
   fprintf(stdout, "New type element %p created with count %d\n", type_element, type_element->count);
   fprintf(stdout, "New type element %p {%p, %p, %ld, %p}\n", 
             type_element, type_element->base_type, type_element->disp_type, type_element->count, type_element->disp);
   counter += map_[i].count;
   type_chunk->push_back(type_element);
   
   if(counter >= chunk_size){
     type_chunks_all.push_back(*type_chunk);
     type_chunk = new std::vector<DataElement*>();
     fprintf(stdout, "Type chunk %p created.\n", type_chunk);
     counter = 0;
   }
   if (counter == 0) 
     delete type_chunk;
 }

 return type_chunks_all;
}
//
// destroys a datatype
//
decaf::
Datatype::~Datatype()
{
  MPI_Type_free(&datatype_);
}

#endif
