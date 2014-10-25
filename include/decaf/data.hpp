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
    void* put_items_; // data pointer to items being put
    int put_nitems_; // number of items being put
    std::vector <unsigned char> get_items_; // all items being gotten
    bool put_self_; // last put was to self
    int err_; // last error

    Data(Datatype dtype) : complete_datatype_(dtype), put_items_(NULL), put_self_(false) {}
    ~Data() {}

    void put_self(bool self) { put_self_ = self; }
    bool put_self() { return put_self_; }
    void put_nitems(int nitems) { put_nitems_ = nitems; }
    int put_nitems() { return put_nitems_; }
    void* put_items() { return put_items_; }
    void put_items(void* d) { put_items_ = d; }
    int get_nitems() { return(get_items_.size() / DatatypeSize(complete_datatype_)); }
    void* get_items() { return(get_items_.size() ? &get_items_[0] : NULL); }
    void err() { ::all_err(err_); }
  };

} // namespace

#endif
