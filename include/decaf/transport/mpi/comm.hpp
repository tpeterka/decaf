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

// communicator constructor forming a communicator from contiguous world ranks
// NB: only collective over the ranks in the range [min_rank, max_rank]
decaf::
Comm::Comm(CommHandle world_comm, int min_rank, int max_rank) : min_rank_(min_rank)
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
Comm::put(Data *data, int dest)
{
  // TODO: prepend typemap?

  // short-circuit put to self
  if (world_rank(dest) == world_rank(rank()))
    data->put_self(true);
  else
  {
    MPI_Request req;
    MPI_Isend(data->data_ptr(), data->put_nitems(), data->complete_datatype_, dest, 0, handle_,
              &req);
  }
  // TODO: wait or test at the very end to complete any remaining requests
}

void
decaf::
Comm::get(Data* data)
{
  if (data->put_self())
    data->put_self(false);
  else
  {
    // TODO: read type from typemap instead of argument?
    MPI_Status status;
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, handle_, &status);
    int nitems; // number of items (of type dtype) in the message
    MPI_Get_count(&status, data->complete_datatype_, &nitems);
    MPI_Aint extent; // datatype size in bytes
    MPI_Type_extent(data->complete_datatype_, &extent);
    int old_size = data->get_items_.size();
    data->get_items_.resize(data->get_items_.size() + nitems * extent);
    MPI_Recv(&data->get_items_[0] + old_size, nitems, data->complete_datatype_, status.MPI_SOURCE,
             0, handle_, &status);
  }
}

#endif
