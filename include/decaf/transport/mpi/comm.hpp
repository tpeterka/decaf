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

#include "../../comm.hpp"
#include <mpi.h>
#include <stdio.h>

// use this version of communicator constructor when splitting a world communicator
// NB: collective over all ranks of world_comm
decaf::
Comm::Comm(CommHandle world_comm, CommType type, int world_rank)
{
  MPI_Comm_split(world_comm, type, world_rank, &handle_);

  MPI_Comm_rank(handle_, &rank_);
  MPI_Comm_size(handle_, &size_);
}

// use this version of communicator constructor when forming a communicator from contiguous ranks
// NB: only collective over the ranks in the range [min_rank, max_rank]

decaf::
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

decaf::
Comm::~Comm()
{
  MPI_Comm_free(&handle_);
}

void
decaf::
Comm::put(void* addr, int num, int dest, Datatype dtype)
{
  MPI_Request req;

  // TODO: prepend typemap?

  MPI_Isend(addr, num, dtype, dest, 0, handle_, &req);

  // TODO: technically, ought to wait or test to complete the isend?
}

void
decaf::
Comm::get(Data& data)
{
  MPI_Status status;

  // TODO: read type from typemap instead of argument?

  MPI_Probe(MPI_ANY_SOURCE, 0, handle_, &status);

  // allocate
  int nitems; // number of items (of type dtype) in the message
  MPI_Get_count(&status, data.complete_datatype_, &nitems);
  MPI_Aint extent; // datatype size in bytes
  MPI_Type_extent(data.complete_datatype_, &extent);
  int old_size = data.items_.size();
  data.items_.resize(data.items_.size() + nitems * extent);
  MPI_Recv(&data.items_[0] + old_size, nitems, data.complete_datatype_, status.MPI_SOURCE, 0,
           handle_, &status);
}

#endif
