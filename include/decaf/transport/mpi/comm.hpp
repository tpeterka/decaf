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

#ifndef DECAF_TRANSPORT_MPI_COMM_HPP
#define DECAF_TRANSPORT_MPI_COMM_HPP

#include <mpi.h>
#include <stdio.h>

using namespace decaf;

// standalone utility, not part of a class
void WorldOrder(CommHandle world_comm, int& world_rank, int& world_size)
{
  MPI_Comm_rank(world_comm, &world_rank);
  MPI_Comm_size(world_comm, &world_size);
}

// use this version of communicator constructor when splitting a world communicator
// NB: collective over all ranks of world_comm
Comm::Comm(CommHandle world_comm, CommType type, int world_rank)
{
  MPI_Comm_split(world_comm, type, world_rank, &handle_);

  MPI_Comm_rank(handle_, &rank_);
  MPI_Comm_size(handle_, &size_);
}

// use this version of communicator constructor when forming a communicator from contiguous ranks
// NB: only collective over the ranks in the range [min_rank, max_rank]
Comm::Comm(CommHandle world_comm, int min_rank, int max_rank)
{
  MPI_Group group, newgroup;
  int range[3];
  range[0] = min_rank;
  range[1] = max_rank;
  range[2] = 1;
  MPI_Comm_group(world_comm, &group);
  MPI_Group_range_incl(group, 1, &range, &newgroup);
  MPI_Comm_create_group(world_comm, newgroup, 0, &handle_);

  MPI_Comm_rank(handle_, &rank_);
  MPI_Comm_size(handle_, &size_);
}

Comm::~Comm()
{
  MPI_Comm_free(&handle_);
}

void
Comm::put(void* addr, int num, Datatype dtype)
{
  MPI_Request req;

  // TODO: prepend size and typemap

  // TODO: looping over all destinations for now, need to specify destinations more
  // carefully (or broadcast?)
  for (int dest = 0; dest < size_; dest++)
    MPI_Isend(addr, num, dtype, dest, 0, handle_, &req);

  // TODO: technically, ought to wait or test to complete the isend?
}

void*
Comm::get(int num, Datatype dtype)
{
  MPI_Request req;
  MPI_Status status;

  // TODO: read type from typemap instead of argument?

  MPI_Probe(MPI_ANY_SOURCE, 0, handle_, &status);

  // allocate
  int nbytes; // number of bytes in the message
  MPI_Get_count(&status, MPI_BYTE, &nbytes);
  unsigned char* addr = new unsigned char[nbytes];
  MPI_Recv(addr, 1, dtype, status.MPI_SOURCE, 0, handle_, &status);
  return addr;
}

#endif
