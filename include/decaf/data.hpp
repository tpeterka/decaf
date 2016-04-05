// DEPRECATED
// //---------------------------------------------------------------------------
// //
// // data interface
// //
// // Tom Peterka
// // Argonne National Laboratory
// // 9700 S. Cass Ave.
// // Argonne, IL 60439
// // tpeterka@mcs.anl.gov
// //
// //--------------------------------------------------------------------------

// #ifndef DECAF_DATA_HPP
// #define DECAF_DATA_HPP

// #include "decaf/types.hpp"
// #include <math.h>
// #include <string.h>
// #include <vector>

// // transport layer implementations
// #ifdef TRANSPORT_MPI
// #include "decaf/transport/mpi/types.h"
// #include "decaf/transport/mpi/tools.hpp"
// #endif

// using namespace std;

// namespace decaf
// {
//     // datatypes
//     class Datatype
//     {
//     public:
//         ~Datatype();                         // TODO: virtual?
//         CommDatatype* comm_datatype()        { return &datatype_; }
//     protected:
//         CommDatatype datatype_;
//         bool absolute_address; // whether the addressing in the datatype is absolute or relative
//     };
//     struct VectorDatatype: Datatype
//     {
//         VectorDatatype(int num_elems,
//                        int stride,
//                        CommDatatype base_type);
//     };
//     struct SliceDatatype: Datatype
//     {
//         SliceDatatype(int dim,
//                       int* full_size,
//                       int* sub_size,
//                       int* start_pos,
//                       CommDatatype base_type);
//     };
//     struct StructDatatype: Datatype
//     {
//         StructDatatype(Address base_addr,
//                        int num_elems,
//                        DataMap* map);
//         std::vector<std::vector<DataMap*> > split(int n);
//     private:
//         DataMap* map_;
//         size_t map_num_elems_;
//     };

//     // data description
//     class Data
//     {
//     public:
//         const CommDatatype complete_datatype_;
//         // TODO: uncomment the following once we start to use them
//         //     const CommDatatype chunk_datatype_;
//         //     const enum Decomposition decomp_type_;

//         Data(CommDatatype dtype) :
//             complete_datatype_(dtype),
//             put_items_(NULL)                 {}
//         ~Data()                              {}
//         void put_nitems(int nitems)          { put_nitems_ = nitems; }
//         int put_nitems()                     { return put_nitems_; }
//         void* put_items()                    { return put_items_; }
//         void put_items(void* d)              { put_items_ = d; }
//         int get_nitems(TaskType task_type);
//         void* get_items(TaskType task_type);
//         void err()                           { ::all_err(err_); }
//         void flush()                         { dflow_get_items_.clear(); con_get_items_.clear();}
//         unsigned char* resize_get_items(int extra_bytes,
//                                         TaskType task_type);
//     private:
//         std::vector <unsigned char> dflow_get_items_; // incoming items for a dataflow process
//         std::vector <unsigned char> con_get_items_;   // incoming items for a consumer process
//         void* put_items_;                             // data pointer to items being put
//         int put_nitems_;                              // number of items being put
//         int err_;                                     // last error
//     };


// } // namespace

// int
// decaf::
// Data::get_nitems(TaskType task_type)
// {
//     if (task_type == DECAF_DFLOW)
//         return(dflow_get_items_.size() / DatatypeSize(complete_datatype_));
//     else
//         return(con_get_items_.size() / DatatypeSize(complete_datatype_));
// }

// void*
// decaf::
// Data::get_items(TaskType task_type)
// {
//     if (task_type == DECAF_DFLOW)
//         return(dflow_get_items_.size() ? &dflow_get_items_[0] : NULL);
//     else
//         return(con_get_items_.size() ? &con_get_items_[0] : NULL);
// }

// // resizes (grows and changes size) get_items by extra_bytes
// // returns pointer to start of extra space
// unsigned char*
// decaf::
// Data::resize_get_items(int extra_bytes,
//                        TaskType task_type)
// {
//     if (task_type == DECAF_DFLOW)
//     {
//         int old_size = dflow_get_items_.size();   // bytes
//         dflow_get_items_.resize(old_size + extra_bytes);
//         return &dflow_get_items_[old_size];
//     }
//     else
//     {
//         int old_size = con_get_items_.size();     // bytes
//         con_get_items_.resize(old_size + extra_bytes);
//         return &con_get_items_[old_size];
//     }
// }

// #endif
