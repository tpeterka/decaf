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
  vector <MPI_Aint> addrs(num_elems, base_addr);
  vector <int> counts(num_elems, 0);
  vector <MPI_Datatype> base_types(num_elems, 0);

  // traverse the map, skipping elements with 0 count, and filling the above vectors
  int nelems = 0;
  for (int i = 0; i < nelems; i++)
  {
    if (map[i].count <= 0)
      continue;
    if (map[i].disp_type == DECAF_OFST)
      addrs[nelems] = addrs[i] + map[i].disp;
    else
      addrs[nelems] = map[i].disp;
    counts[nelems] = map[i].count;
    base_types[nelems] = map[i].base_type;
    nelems++;
  }

  // create the datatype
  MPI_Type_create_struct(nelems, &counts[0], &addrs[0], &base_types[0], &datatype_);

  MPI_Type_commit(&datatype_);
  absolute_address = true;
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
