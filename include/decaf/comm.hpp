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

#ifndef DECAF_COMM_HPP
#define DECAF_COMM_HPP

#include "types.hpp"
#include "data.hpp"
#include <vector>
#include <math.h>

namespace decaf
{

  // generic communication mechanism for producer, consumer, dataflow
  // ranks in communicator are contiguous in the world
  class Comm
  {
  public:
    Comm(CommHandle world_comm, int min_rank, int max_rank, int num_srcs, int num_dests,
         int start_dest);
    ~Comm();
    CommHandle handle() { return handle_; }
    int size() { return size_; }
    int rank() { return rank_; }
    int world_rank(int rank) { return(rank + min_rank); } // world rank of any rank in this comm
    int world_rank() { return(rank_ + min_rank); } // my world rank
    void put(Data* data, int dest, bool forward);
    void get(Data* data, bool aux=false);
    void flush();
    int num_inputs();
    int start_input();
    int num_outputs();
    int start_output();

    // debug
    void test1();

  private:
    CommHandle handle_; // communicator handle in the transport layer
    int size_; // communicator size
    int rank_; // rank in communicator
    int min_rank; // min (world) rank of communicator
    std::vector<CommRequest> reqs; // pending communication requests
    int num_srcs; // number of sources (producers) within the communicator
    int num_dests; // numbers of destinations (consumers) within the communicator
    int start_dest; // first destination rank within the communicator (0 to size_ - 1)
    bool forward_; // last put from this rank was a forward
  };

} // namespace

// returns the number of inputs (sources) expected for a get
int
decaf::
Comm::num_inputs()
{
  int dest_rank = rank_ - num_srcs; // rank starting at the destinations, which start after sources
  float step = (float)num_srcs / (float)num_dests;
  return(ceilf((dest_rank + 1) * step) - floorf(dest_rank * step));
}

// returns the rank of the starting input (sources) expected for a sequence of gets
int
decaf::
Comm::start_input()
{
  int dest_rank = rank_ - start_dest; // rank wrt start of the destinations
  float step = (float)num_srcs / (float)num_dests;
  return(dest_rank * step);
}

// returns the number of outputs (destinations) expected for a sequence of puts
int
decaf::
Comm::num_outputs()
{
  int src_rank = rank_; // rank starting at the sources, which are at the start of the comm
  float step = (float)num_dests / (float)num_srcs;
  return(ceilf((src_rank + 1) * step) - floorf(src_rank * step));
}

// returns the rank of the staring output (destinations) expected for a sequence of puts
int
decaf::
Comm::start_output()
{
  int src_rank = rank_; // rank starting at the sources, which are at the start of the comm
  float step = (float)num_dests / (float)num_srcs;
  return(src_rank * step + start_dest); // wrt start of communicator
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

#endif
