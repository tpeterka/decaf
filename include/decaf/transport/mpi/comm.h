//---------------------------------------------------------------------------
//
// decaf communicator interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#ifndef DECAF_COMM_H
#define DECAF_COMM_H

#include <decaf/types.hpp>
#include <decaf/transport/mpi/types.h>
#include <vector>
#include <math.h>

namespace decaf
{

    // generic communication mechanism for producer, consumer, dataflow
    // ranks in communicator are contiguous in the world
    class Comm
    {
    public:
        Comm(CommHandle world_comm,
             int min_rank,
             int max_rank,
             int num_srcs = 0,
             int num_dests = 0,
             int start_dest = 0,
             CommTypeDecaf comm_type = 0);
        Comm(CommHandle world_comm);
        ~Comm();
        CommHandle handle();
        int size();
        int rank();
        int world_rank(int rank);// world rank of any rank in this comm
        int world_rank();// my world rank
        CommTypeDecaf type();
        int num_inputs();
        int start_input();
        int num_outputs();
        int start_output();

        // debug
        void test1();

    private:
        CommHandle handle_;                  // communicator handle in the transport layer
        int size_;                           // communicator size
        int rank_;                           // rank in communicator
        int min_rank;                        // min (world) rank of communicator
        std::vector<CommRequest> reqs;       // pending communication requests
        int num_srcs;             // number of sources (producers) within the communicator
        int num_dests;            // numbers of destinations (consumers) within the communicator
        int start_dest;           // first destination rank within the communicator (0 to size_ - 1)
        CommTypeDecaf type_;      // communicator type (prod, dflow, con, prod_dflow, or dflow_con)
        bool new_comm_handle_;    // a new low level communictor (handle) was created
    };

} // namespace

#endif
