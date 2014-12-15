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
    std::vector<std::vector<DataElement*> > split(int n);
  private: 
    DataElement* map_;
    size_t map_num_elems_;
  };

  // data description
  class Data
  {
  public:
    const CommDatatype complete_datatype_;
    // TODO: uncomment the following once we start to use them
//     const CommDatatype chunk_datatype_;
//     const enum Decomposition decomp_type_;

    Data(CommDatatype dtype) : complete_datatype_(dtype), put_items_(NULL) {}
    //Data(){}
    ~Data() {}
    void put_nitems(int nitems) { put_nitems_ = nitems; }
    int put_nitems() { return put_nitems_; }
    void* put_items() { return put_items_; }
    void put_items(void* d) { put_items_ = d; }
    int get_nitems() { return(get_items_.size() / DatatypeSize(complete_datatype_)); }
    void* get_items(bool aux = false);
    void err() { ::all_err(err_); }
    void flush() { get_items_.clear(); }
    unsigned char* resize_get_items(int extra_bytes, bool aux = false);
  private:
    std::vector <unsigned char> get_items_; // all items being gotten
    // 2nd vector of all items being gotten is needed when same node is a receiver for both
    // dataflow and consumer with different message paths
    std::vector <unsigned char> aux_get_items_;
    void* put_items_; // data pointer to items being put
    int put_nitems_; // number of items being put
    int err_; // last error
  };

  template <class T>
  class DataBis
  {
  public:
    DataBis(void (*create_datatype)(const T*, int*, DataElement*, MPI_Datatype*)) : create_datatype(create_datatype) {}
    vector<T> getData() { return data;}
    T** getPointerData() { return data.empty() ? NULL : &data[0];}
    int getNumberElements() { return data.size();} 
    void addDataElement(const T* data_elem) { data.push_back(data_elem);}
    void deleteElement(int index) { data.erase(data.begin()+index);}
    void deleteElements(int from, int to) { data.erase(data.begin()+from, data.begin()+to);}
    int generateMPIDatatype(const T* data_elem, MPI_Datatype* mpi_map) { create_datatype(data_elem, NULL, NULL, mpi_map); } 
    int generateMap(const T* data_elem, int* map_count, DataElement* map) { create_datatype(data_elem, map_count, map, NULL);}
    static void getMPIDatatypeFromMap(int map_count, const DataElement* map, MPI_Datatype* mpi_map);
    //vector<pair<DataElement, i> > split(int count)
  private:
    vector<T> data;
    void (*create_datatype)(const T* , int*, DataElement*, MPI_Datatype*);
  };

} // namespace

// returns pointer to start of get_items
void*
decaf::
Data::get_items(bool aux)
{
  if (aux)
    return(aux_get_items_.size() ? &aux_get_items_[0] : NULL);
  else
    return(get_items_.size() ? &get_items_[0] : NULL);
}

// resizes (grows and changes size) get_items by extra_bytes
// returns pointer to start of extra space
unsigned char*
decaf::
Data::resize_get_items(int extra_bytes, bool aux)
{
  if (aux)
  {
    int old_size = aux_get_items_.size(); // bytes
    aux_get_items_.resize(old_size + extra_bytes);
    return &aux_get_items_[old_size];
  }
  else
  {
    int old_size = get_items_.size(); // bytes
    get_items_.resize(old_size + extra_bytes);
    return &get_items_[old_size];
  }
}

#endif
