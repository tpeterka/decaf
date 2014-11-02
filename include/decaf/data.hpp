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

using namespace std;

namespace decaf
{
  // datatypes
  class Datatype
  {
  public:
    ~Datatype();     // TODO: virtual?
    CommDatatype* comm_datatype() { return &datatype_; }
  protected:
    CommDatatype datatype_;
    bool absolute_address; // whether the addressing in the datatype is absolute or relative
  };
  struct VectorDatatype: Datatype
  {
    VectorDatatype(int num_elems, int stride, CommDatatype base_type);
  };
  struct SliceDatatype: Datatype
  {
    SliceDatatype(int dim, int* full_size, int* sub_size, int* start_pos, CommDatatype base_type);
  };
  struct StructDatatype: Datatype
  {
    StructDatatype(Address base_addr, int num_elems, DataElement* map);
  };

  // data description
  class Data
  {
  public:
    const CommDatatype complete_datatype_;
    // TODO: uncomment the following once we start to use them
//     const CommDatatype chunk_datatype_;
//     const enum Decomposition decomp_type_;

    Data(CommDatatype dtype) : complete_datatype_(dtype), put_items_(NULL), put_self_(false) {}
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
    void flush() { get_items_.clear(); }
    unsigned char* resize_get_items(int extra_bytes);
  private:
    std::vector <unsigned char> get_items_; // all items being gotten
    void* put_items_; // data pointer to items being put
    int put_nitems_; // number of items being put
    bool put_self_; // last put was to self
    int err_; // last error
  };

} // namespace

// resizes (grows and changes size) get_items by extra_bytes
// returns pointer to start of extra space
unsigned char*
decaf::
Data::resize_get_items(int extra_bytes)
{
  int old_size = get_items_.size(); // bytes
  get_items_.resize(old_size + extra_bytes);
  return &get_items_[old_size];
}

#endif
