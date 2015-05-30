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
#include <math.h>
#include <string.h>
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
        ~Datatype();                         // TODO: virtual?
        CommDatatype* comm_datatype()        { return &datatype_; }
    protected:
        CommDatatype datatype_;
        bool absolute_address; // whether the addressing in the datatype is absolute or relative
    };
    struct VectorDatatype: Datatype
    {
        VectorDatatype(int num_elems,
                       int stride,
                       CommDatatype base_type);
    };
    struct SliceDatatype: Datatype
    {
        SliceDatatype(int dim,
                      int* full_size,
                      int* sub_size,
                      int* start_pos,
                      CommDatatype base_type);
    };
    struct StructDatatype: Datatype
    {
        StructDatatype(Address base_addr,
                       int num_elems,
                       DataMap* map);
        std::vector<std::vector<DataMap*> > split(int n);
    private:
        DataMap* map_;
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

        Data(CommDatatype dtype) :
            complete_datatype_(dtype),
            put_items_(NULL)                 {}
        ~Data()                              {}
        void put_nitems(int nitems)          { put_nitems_ = nitems; }
        int put_nitems()                     { return put_nitems_; }
        void* put_items()                    { return put_items_; }
        void put_items(void* d)              { put_items_ = d; }
        int get_nitems(TaskType task_type);
        void* get_items(TaskType task_type);
        void err()                           { ::all_err(err_); }
        void flush()                         { dflow_get_items_.clear(); con_get_items_.clear();}
        unsigned char* resize_get_items(int extra_bytes,
                                        TaskType task_type);
    private:
        std::vector <unsigned char> dflow_get_items_; // incoming items for a dataflow process
        std::vector <unsigned char> con_get_items_;   // incoming items for a consumer process
        void* put_items_;                             // data pointer to items being put
        int put_nitems_;                              // number of items being put
        int err_;                                     // last error
    };

    // DEPRECATED
    //     template <class T>
    //     class DataBis
    //     {
    //     public:
    //         DataBis(void (*create_datatype)(const T*, int*, DataMap**, CommDatatype*)) 
    //             : create_datatype(create_datatype) {}
    //         vector<T> getData() { return data;}
    //         T* getPointerData() { return data.empty() ? NULL : &data[0];}
    //         int getNumberElements() { return data.size();}
    //         void addDataMap(const T* data_elem) { data.push_back(data_elem);}
    //         void deleteElement(int index) { data.erase(data.begin()+index);}
    //         void deleteElements(int from, int to) { 
    //             data.erase(data.begin()+from, data.begin()+to);
    //         }
    //         int generateCommDatatype(const T* data_elem, CommDatatype* comm_datatype) { 
    //             create_datatype(data_elem, NULL, NULL, comm_datatype); 
    //         }
    //         int generateMap(const T* data_elem, int* map_count, DataMap** map) { 
    //             create_datatype(data_elem, map_count, map, NULL);
    //         }
    //         void getCommDatatypeFromMap(int map_count, DataMap* map, 
    //                                     CommDatatype* comm_datatype) {
    //             StructDatatype* s_type = new StructDatatype((size_t) 0, map_count, map);
    //             *comm_datatype = *(s_type->comm_datatype());
    //         }
    //         vector<vector<DataMap*> > split(int map_count, DataMap* map, int count){
    //             // debug string
    //             char debug[DECAF_DEBUG_MAX];			
    //             // get the total size of the Struct
    //             int total_size_b = 0;
    //             for( int i=0; i<map_count ; i++)
    //                 total_size_b += map[i].count*DatatypeSize(map[i].base_type);
    //             sprintf(debug, "Total Struct typemap is %d\n", total_size_b);
    //             all_dbg(stderr, debug);

    //             // chunk size
    //             // TODO test if total size is actually > than n 
    //             int chunk_size_b = ceil((float)total_size_b/(float)count);
    //             sprintf(debug, "Chunk size is %d\n", chunk_size_b);
    //             all_dbg(stderr, debug);

    //             vector<vector<DataMap*> > type_chunks_all;
    //             vector<DataMap*>* type_chunk;
    //             DataMap* type_element = new DataMap();
    //             size_t size;
    //             int counter_b = 0;
    //             type_chunk = new vector<DataMap*>(); // allocate the first datatype chunk
    //             sprintf(debug, "Type chunk %p created.\n", type_chunk);
    //             all_dbg(stderr, debug);
    //             // loop over the OriginalDataMap to create new one
    //             for( int i=0; i<map_count; i++) {
    //                 int element_count_b = map[i].count*DatatypeSize(map[i].base_type);
    //                 int remaining_elt = map[i].count;
    //                 size = DatatypeSize(map[i].base_type);
    //                 // decide if this type element wil be splitted
    //                 while(counter_b + remaining_elt*size > chunk_size_b){
    //                     type_element = new DataMap();
    //                     memcpy(type_element, &map[i], sizeof(DataMap));// do not keep this
    //                     // set the right count
    //                     type_element->count = ceil( (chunk_size_b-counter_b) / size); 
    //                     // set the right displacement/address
    //                     type_element->disp += (map[i].count - remaining_elt)*size;
    //                     // we finished  a type chunk
    //                     sprintf(debug, "New type element %p created with count %d\n", 
    //                             type_element, type_element->count);
    //                     all_dbg(stderr, debug);
    //                     sprintf(debug, "New type element %p {0x%X, 0x%X, %d, 0x%lX}\n",
    //                             type_element, type_element->base_type, 
    //                             type_element->disp_type, type_element->count, 
    //                             type_element->disp);
    //                     all_dbg(stderr, debug);
    //                     type_chunk->push_back(type_element);
    //                     type_chunks_all.push_back(*type_chunk);
    //                     type_chunk = new std::vector<DataMap*>();
    //                     sprintf(debug, "Type chunk %p created.\n", type_chunk);
    //                     all_dbg(stderr, debug);
    //                     counter_b = 0;
    //                     remaining_elt -= type_element->count;
    //                 }

    //                 if (remaining_elt != 0 && remaining_elt != map[i].count){
    //                     type_element = new DataMap();
    //                     memcpy(type_element, &map[i], sizeof(DataMap));// do not keep this
    //                     type_element->count = remaining_elt;
    //                     type_element->disp += (map[i].count - remaining_elt)*size;
    //                     sprintf(debug, "New type element %p created with count %d\n", 
    //                             type_element, type_element->count);
    //                     all_dbg(stderr, debug);
    //                     sprintf(debug, "New type element %p {0x%X, 0x%X, %d, 0x%lX}\n",
    //                             type_element, type_element->base_type, 
    //                             type_element->disp_type, type_element->count, 
    //                             type_element->disp);
    //                     all_dbg(stderr, debug);
    //                     counter_b += remaining_elt*size;
    //                     type_chunk->push_back(type_element);
    //                     continue;
    //                 }

    //                 type_element = new DataMap();
    //                 memcpy(type_element, &map[i], sizeof(DataMap));// do not keep this
    //                 sprintf(debug, "New type element %p created with count %d\n", 
    //                         type_element, type_element->count);
    //                 all_dbg(stderr, debug);
    //                 sprintf(debug, "New type element %p {0x%X, 0x%X, %d, 0x%lX}\n",
    //                         type_element, type_element->base_type, 
    //                         type_element->disp_type, type_element->count, 
    //                         type_element->disp);
    //                 all_dbg(stderr, debug);
    //                 counter_b += element_count_b;
    //                 type_chunk->push_back(type_element);

    //                 if(counter_b >= chunk_size_b || i == map_count-1){
    //                     type_chunks_all.push_back(*type_chunk);
    //                     if ( i != map_count-1){
    //                         type_chunk = new std::vector<DataMap*>();
    //                         sprintf(debug, "Type chunk %p created.\n", type_chunk);
    //                         all_dbg(stderr, debug);
    //                     }
    //                     counter_b = 0;
    //                 }
    //             }
    //             return type_chunks_all;
    //         }
    //     private:
    //         vector<T> data;
    //         void (*create_datatype)(const T* , int*, DataMap**, CommDatatype*);
    //     };

} // namespace

int
decaf::
Data::get_nitems(TaskType task_type)
{
    if (task_type == DECAF_DFLOW)
        return(dflow_get_items_.size() / DatatypeSize(complete_datatype_));
    else
        return(con_get_items_.size() / DatatypeSize(complete_datatype_));
}

void*
decaf::
Data::get_items(TaskType task_type)
{
    if (task_type == DECAF_DFLOW)
        return(dflow_get_items_.size() ? &dflow_get_items_[0] : NULL);
    else
        return(con_get_items_.size() ? &con_get_items_[0] : NULL);
}

// resizes (grows and changes size) get_items by extra_bytes
// returns pointer to start of extra space
unsigned char*
decaf::
Data::resize_get_items(int extra_bytes,
                       TaskType task_type)
{
    if (task_type == DECAF_DFLOW)
    {
        int old_size = dflow_get_items_.size();   // bytes
        dflow_get_items_.resize(old_size + extra_bytes);
        return &dflow_get_items_[old_size];
    }
    else
    {
        int old_size = con_get_items_.size();     // bytes
        con_get_items_.resize(old_size + extra_bytes);
        return &con_get_items_[old_size];
    }
}

#endif
