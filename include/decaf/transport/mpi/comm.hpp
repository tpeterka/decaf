//---------------------------------------------------------------------------
//
// decaf communicator implementation in mpi transport layer
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// forms a deaf communicator from contiguous world ranks of an MPI communicator
// only collective over the ranks in the range [min_rank, max_rank]
decaf::
Comm::Comm(CommHandle world_comm,
           int min_rank,
           int max_rank,
           int num_srcs,
           int num_dests,
           int start_dest,
           CommTypeDecaf comm_type):
    min_rank(min_rank),
    num_srcs(num_srcs),
    num_dests(num_dests),
    start_dest(start_dest),
    type_(comm_type)
{
    MPI_Group group, newgroup;
    int range[3];
    range[0] = min_rank;
    range[1] = max_rank;
    range[2] = 1;
    MPI_Comm_group(world_comm, &group);
    MPI_Group_range_incl(group, 1, &range, &newgroup);
    MPI_Comm_create_group(world_comm, newgroup, 0, &handle_);
    MPI_Group_free(&group);
    MPI_Group_free(&newgroup);

    MPI_Comm_rank(handle_, &rank_);
    MPI_Comm_size(handle_, &size_);
    new_comm_handle_ = true;
}

// wraps a decaf communicator around an entire MPI communicator
decaf::
Comm::Comm(CommHandle world_comm):
    handle_(world_comm),
    min_rank(0)
{
    num_srcs         = 0;
    num_dests        = 0;
    start_dest       = 0;
    type_            = 0;
    new_comm_handle_ = false;

    MPI_Comm_rank(handle_, &rank_);
    MPI_Comm_size(handle_, &size_);
}

decaf::
Comm::~Comm()
{
    if (new_comm_handle_)
        MPI_Comm_free(&handle_);
}

#endif
