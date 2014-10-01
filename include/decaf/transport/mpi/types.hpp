//---------------------------------------------------------------------------
//
// decaf mpi transport layer
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_TRANSPORT_MPI_TYPES_HPP
#define DECAF_TRANSPORT_MPI_TYPES_HPP

#include<mpi.h>

typedef MPI_Comm CommHandle;
typedef MPI_Datatype Datatype;

// standalone utilities, not part of a class
void WorldOrder(CommHandle world_comm, int& world_rank, int& world_size)
{
  MPI_Comm_rank(world_comm, &world_rank);
  MPI_Comm_size(world_comm, &world_size);
}

size_t DatatypeSize(Datatype dtype)
{
  MPI_Aint extent;
  MPI_Type_extent(dtype, &extent);
  return extent;
}

#endif
