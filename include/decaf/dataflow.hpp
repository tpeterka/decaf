//---------------------------------------------------------------------------
// DEPRECATED
// dataflow interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

// #ifndef DECAF_DATAFLOW_HPP
// #define DECAF_DATAFLOW_HPP

// #include "types.hpp"
// #include "comm.hpp"

// // transport layer implementations
// #ifdef TRANSPORT_MPI
// #include "transport/mpi/comm.hpp"
// #endif

// namespace decaf
// {

//   // dataflow
//   struct Dataflow
//   {
//     int err_; // last error

//     Dataflow(Comm* prod_dflow_comm, Comm* dflow_con_comm, Data& data, int nsteps);
//     ~Dataflow() {}

//     void err() { ::all_err(err_); }
//   };

// } // namespace

// Dataflow::Dataflow(Comm* prod_dflow_comm, Comm* dflow_con_comm, Data& data, int nsteps)
// {
//   // relay the data
//   // TODO: this is where different patterns of primitives would be used, for now simple direct link

//   // TODO: following loop does not include pipeline chunks yet
//   for (int i = 0; i < nsteps; i++)
//   {
//     data.base_addr_ = prod_dflow_comm->get(1, data.complete_datatype_);
//     dflow_con_comm->put(data.base_addr_, 1, data.complete_datatype_);

//     // data_ptr was allocated in Comm:get()
//     delete[] (unsigned char*)data.base_addr_;
//   }
// }
// #endif
