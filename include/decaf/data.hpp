//---------------------------------------------------------------------------
//
// data interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATA_HPP
#define DECAF_DATA_HPP

#include "types.hpp"
#include <vector>

// transport layer implementations
#ifdef TRANSPORT_MPI
#include "transport/mpi/types.hpp"
#endif

namespace decaf
{

  // intrinsic data description
  struct Data
  {
    Datatype complete_datatype_;
    Datatype chunk_datatype_;
    enum Decomposition decomp_type_;
    std::vector <unsigned char> items_; // all items in contiguous order
    void* dp_; // data pointer when items is not used
    int err_; // last error

    Data(Datatype dtype) : complete_datatype_(dtype), dp_(NULL) {}
    ~Data() {}

    void* data_ptr() { return(items_.size() ? &items_[0] : dp_); }
    int nitems() { return items_.size() / DatatypeSize(complete_datatype_); }
    void err() { ::all_err(err_); }

  };

} // namespace

#endif
