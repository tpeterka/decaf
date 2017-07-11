#include <bredala/transport/mpi/comm.h>
#include <cstdio>

CommHandle
decaf::
Comm::handle()
{
    return handle_;
}

int
decaf::
Comm::size()
{
    return size_;
}

int
decaf::
Comm::rank()
{
    return rank_;
}

int
decaf::
Comm::world_rank(int rank)
{
    return(rank + min_rank);
}// world rank of any rank in this comm

int
decaf::
Comm::world_rank()
{
    return(rank_ + min_rank);
}// my world rank

// returns the number of inputs (sources) expected for a get
int
decaf::
Comm::num_inputs()
{
    // rank starting at the destinations, which start after sources
    int dest_rank = rank_ - num_srcs;
    float step = (float)num_srcs / (float)num_dests;
    return(ceilf((dest_rank + 1) * step) - floorf(dest_rank * step));
}

// returns the rank of the starting input (sources) expected for a sequence of gets
int
decaf::
Comm::start_input()
{
    // rank wrt start of the destinations
    int dest_rank = rank_ - start_dest;
    float step = (float)num_srcs / (float)num_dests;
    return(dest_rank * step);
}

// returns the number of outputs (destinations) expected for a sequence of puts
int
decaf::
Comm::num_outputs()
{
    // rank starting at the sources, which are at the start of the comm
    int src_rank = rank_;
    float step = (float)num_dests / (float)num_srcs;
    return(ceilf((src_rank + 1) * step) - floorf(src_rank * step));
}

// returns the rank of the staring output (destinations) expected for a sequence of puts
int
decaf::
Comm::start_output()
{
    // rank starting at the sources, which are at the start of the comm
    int src_rank = rank_;
    float step = (float)num_dests / (float)num_srcs;
    return(src_rank * step + start_dest);    // wrt start of communicator
}

// debug
// unit test for num_inputs, num_outputs, start_output
// sets different combinations of num_srcs, num_dests, and ranks and prints the
// outputs of num_inputs, num_outputs, start_output
void
decaf::
Comm::test1()
{
    for (num_srcs = 1; num_srcs <= 5; num_srcs++)
    {
        for (num_dests = 1; num_dests <= 5; num_dests++)
        {
            size_ = num_srcs + num_dests;
            for (rank_ = 0; rank_ < num_srcs; rank_++)
                fprintf(stderr, "num_srcs %d num_dests %d src_rank %d num_outputs %d start_output %d\n",
                        num_srcs, num_dests, rank_, num_outputs(), start_output() - num_srcs);
            for (rank_ = num_srcs; rank_ < num_srcs + num_dests; rank_++)
                fprintf(stderr, "num_srcs %d num_dests %d dest_rank %d num_inputs %d start_input %d\n",
                        num_srcs, num_dests, rank_ - num_srcs, num_inputs(), start_input());
            fprintf(stderr, "\n");
        }
    }
}

// forms a deaf communicator from contiguous world ranks of an MPI communicator
// only collective over the ranks in the range [min_rank, max_rank]
decaf::
Comm::Comm(CommHandle world_comm,
           int min_rank,
           int max_rank,
           int num_srcs,
           int num_dests,
           int start_dest)://,
    min_rank(min_rank),
    num_srcs(num_srcs),
    num_dests(num_dests),
    start_dest(start_dest)
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


// Previously in decaf/include/bredala/transport/mpi/tools.hpp
int
decaf::CommRank(CommHandle comm)
{
    int rank;
    MPI_Comm_rank(comm, &rank);
    return rank;
}

int
decaf::CommSize(CommHandle comm)
{
    int size;
    MPI_Comm_size(comm, &size);
    return size;
}

size_t
decaf::DatatypeSize(CommDatatype dtype)
{
    MPI_Aint extent;
    MPI_Aint lb;
    MPI_Type_get_extent(dtype, &lb, &extent);
    return extent;
}

Address
decaf::addressof(const void *addr)
{
    MPI_Aint p;
    MPI_Get_address(addr, &p);
    return p;
}
