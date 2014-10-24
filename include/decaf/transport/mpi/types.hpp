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
typedef MPI_Request CommRequest;

// standalone utilities, not part of a class

int CommRank(CommHandle comm)
{
  int rank;
  MPI_Comm_rank(comm, &rank);
  return rank;
}

int CommSize(CommHandle comm)
{
  int size;
  MPI_Comm_size(comm, &size);
  return size;
}

size_t DatatypeSize(Datatype dtype)
{
  MPI_Aint extent;
  MPI_Type_extent(dtype, &extent);
  return extent;
}

#endif
