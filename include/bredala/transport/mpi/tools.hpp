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

#ifndef DECAF_TRANSPORT_MPI_TOOLS_HPP
#define DECAF_TRANSPORT_MPI_TOOLS_HPP

#include <mpi.h>
#include <bredala/transport/mpi/types.h>

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

size_t DatatypeSize(CommDatatype dtype)
{
    MPI_Aint extent;
    MPI_Aint lb;
    MPI_Type_get_extent(dtype, &lb, &extent);
    return extent;
}

Address addressof(const void *addr)
{
    MPI_Aint p;
    MPI_Get_address(addr, &p);
    return p;
}

#endif
