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

typedef MPI_Comm        CommHandle;
typedef MPI_Datatype    CommDatatype;
typedef MPI_Request     CommRequest;
typedef MPI_Aint        Address;
typedef MPI_Request     Request;

#endif
