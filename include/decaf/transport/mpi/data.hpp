// DEPRECATED

// //---------------------------------------------------------------------------
// //
// // decaf datatypes implementation in mpi transport layer
// //
// // Tom Peterka
// // Argonne National Laboratory
// // 9700 S. Cass Ave.
// // Argonne, IL 60439
// // tpeterka@mcs.anl.gov
// //
// //--------------------------------------------------------------------------

// #ifndef DECAF_TRANSPORT_MPI_DATA_HPP
// #define DECAF_TRANSPORT_MPI_DATA_HPP

// #include "../../data.hpp"
// #include <mpi.h>
// #include <stdio.h>

// //
// // creates a vector datatype
// //
// // num_elems: number of elements in the vector
// // stride: number of elements between start of each element (usually 1)
// // base_type: data type of vector elements
// //
// decaf::
// VectorDatatype::
// VectorDatatype(int num_elems, int stride, CommDatatype base_type)
// {
//     MPI_Type_vector(num_elems, 1, stride, base_type, &datatype_);
//     MPI_Type_commit(&datatype_);
//     absolute_address = false;
// }
// //
// // creates a subarray datatype
// //
// // dim: number of elements in full_size, start_pos, sub_size
// // full_size: full sizes of the array ([x][y][z] (not C) order)
// // start_pos: starting indices of the array ([x][y][z] (not C) order)
// // sub_size: desired sizes of subarray ([x][y][z] (not C) order)
// // base_type: data type of array elements
// //
// decaf::
// SliceDatatype::
// SliceDatatype(int dim, int* full_size, int* sub_size, int* start_pos, CommDatatype base_type)
// {
//     // fortran order below is not a bug: I always want [x][y][z] order
//     MPI_Type_create_subarray(dim, full_size, sub_size, start_pos,
//                              MPI_ORDER_FORTRAN, base_type, &datatype_);
//     MPI_Type_commit(&datatype_);
//     absolute_address = false;
// }
// //
// // creates a structure datatype
// //
// // basse_addr: base address added to relative OFST displacements
// // num_elems: number of data elements in the typemap
// // map: typemap
// //
// decaf::
// StructDatatype::
// StructDatatype(Address base_addr, int num_elems, DataMap* map)
// {
//     // debug string
//     char debug[DECAF_DEBUG_MAX];
//     // form the vectors needed to create the struct datatype
//     //vector <MPI_Aint> addrs(num_elems, base_addr);
//     vector<MPI_Aint> addrs;
//     //vector <int> counts(num_elems, 0);
//     vector<int> counts;
//     //vector <MPI_Datatype> base_types(num_elems, 0);
//     vector<MPI_Datatype> base_types;

//     // traverse the map, skipping elements with 0 count, and filling the above vectors
//     int nelems = 0;
//     for (int i = 0; i < num_elems; i++)
//     {
//         if (map[i].count <= 0)
//             continue;
//         // debug
//         int my_rank = -1;
//         MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
//         if (my_rank == 0){
//             sprintf(debug, "[%d] Processing DataMap %d: (0x%X, 0x%X, %d, 0x%lX)\n", 
//                     my_rank, i, map[i].base_type, map[i].disp_type,
//                     map[i].count, map[i].disp);
// 	    all_dbg(stderr, debug);
//         }
//         if (map[i].disp_type == DECAF_OFST){
//             //addrs[nelems] = addrs[i] + map[i].disp;
//             addrs.push_back(base_addr+map[i].disp);
//             map[i].disp += base_addr;
//         }else{
//             //addrs[nelems] = map[i].disp;
//             addrs.push_back(map[i].disp);
//         }
//         //counts[nelems] = map[i].count;
//         counts.push_back(map[i].count);
//         //base_types[nelems] = map[i].base_type;
//         base_types.push_back(map[i].base_type);
//         nelems++;
//     }
//     // create the datatype
//     MPI_Type_create_struct(counts.size(), &counts[0], &addrs[0], &base_types[0], 
//                            &datatype_);
//     //memcpy(&datatype_, del_map, sizeof(del_map));
//     MPI_Type_commit(&datatype_);
//     absolute_address = true;
//     int del_type_size = 0;
//     MPI_Type_size(datatype_, &del_type_size);
//     MPI_Aint* addrs_debug = &addrs[0];

//     if (addrs.size() > 4){
//   	sprintf(debug, "gid=%d, mins[0]= %f, maxs[2]= %f,particles[0]=%f, tets[0].verts[0]=%d\n",
//                 *((int*)addrs[0]), ((float*)addrs[1])[0], 
//                 ((float*)addrs[2])[0], ((float*)addrs[5])[0], -1 );
//         all_dbg(stderr, debug);
//     }
//     // save the map for later datatype processing
//     map_num_elems_ = num_elems;
//     map_ = (DataMap*) malloc(sizeof(DataMap)*num_elems); // TODO: don't use malloc
//     memcpy(map_, map, sizeof(DataMap)*num_elems);
// }

// // split an StructDatatype
// std::vector <std::vector<DataMap*> >
// decaf::
// StructDatatype::
// split(int n)
// {
//     // debug string
//     char debug[DECAF_DEBUG_MAX];
//     // get the total size of the Struct
//     int total_size = 0;
//     for( int i=0; i<map_num_elems_ ; i++)
//         total_size += map_[i].count;
//     sprintf(debug, "Total Struct typemap is %d\n", total_size);
//     all_dbg(stderr, debug);

//     // chunk size
//     // TODO: test if total size is actually > than n
//     int chunk_size = ceil((float)total_size/(float)n); 
//     sprintf(debug, "Chunk size is %d\n", chunk_size);
//     all_dbg(stderr, debug);

//     std::vector<std::vector<DataMap*> > type_chunks_all;
//     std::vector<DataMap*>* type_chunk;
//     DataMap* type_element = new DataMap();
//     memcpy(type_element, &map_[0], sizeof(DataMap));// do not keep this
//     int counter = 0;
//     type_chunk = new std::vector<DataMap*>(); // allocate the first datatype chunk
//     sprintf(debug, "Type chunk %p created.\n", (void*)type_chunk);
//     all_dbg(stderr, debug);
//     // loop over the OriginalDataMap to create new one
//     for( int i=0; i<map_num_elems_; i++) {
//         int element_count = map_[i].count;
//         // decide if this type elemenet wil be splitted
//         while(counter + element_count > chunk_size){
//             type_element = new DataMap();
//             memcpy(type_element, &map_[i], sizeof(DataMap));// do not keep this
//             type_element->count = chunk_size - counter; // set the right count
//             // we finished  a type chunk
//             sprintf(debug, "New type element %p created with count %d\n", 
//                     (void*)type_element, type_element->count);
//             all_dbg(stderr, debug);
//             sprintf(debug, "New type element %p {0x%X, 0x%X, %d, 0x%lX}\n",
//                     (void*)type_element, type_element->base_type, 
//                     type_element->disp_type, type_element->count, 
//                     type_element->disp);
//             all_dbg(stderr, debug);
//             type_chunk->push_back(type_element);
//             type_chunks_all.push_back(*type_chunk);
//             type_chunk = new std::vector<DataMap*>();
//             sprintf(debug, "Type chunk %p created.\n", (void*)type_chunk);
//             all_dbg(stderr, debug);
//             counter = 0;
//             element_count -= type_element->count;
//         }

//         if (element_count != 0 && element_count != map_[i].count){
//             type_element = new DataMap();
//             memcpy(type_element, &map_[i], sizeof(DataMap));// do not keep this
//             type_element->count = element_count;
//             sprintf(debug, "New type element %p created with count %d\n", 
//                     (void*)type_element, type_element->count);
//             all_dbg(stderr, debug);
//             sprintf(debug, "New type element %p {0x%X, 0x%X, %d, 0x%lX}\n",
//                     type_element, type_element->base_type, 
//                     type_element->disp_type, type_element->count, 
//                     type_element->disp);
//             all_dbg(stderr, debug);
//             counter += element_count;
//             type_chunk->push_back(type_element);
//             continue;
//         }

//         type_element = new DataMap();
//         memcpy(type_element, &map_[i], sizeof(DataMap));// do not keep this
//         sprintf(debug, "New type element %p created with count %d\n", 
//                 (void*)type_element, type_element->count);
//         all_dbg(stderr, debug);
//         sprintf(debug, "New type element %p {0x%X, 0x%X, %d, 0x%lX}\n",
//                 type_element, type_element->base_type, 
//                 type_element->disp_type, type_element->count, 
//                 type_element->disp);
//         all_dbg(stderr, debug);
//         counter += map_[i].count;
//         type_chunk->push_back(type_element);

//         if(counter >= chunk_size){
//             type_chunks_all.push_back(*type_chunk);
//             type_chunk = new std::vector<DataMap*>();
//             sprintf(debug, "Type chunk %p created.\n", (void*)type_chunk);
//             all_dbg(stderr, debug);
//             counter = 0;
//         }
//         if (counter == 0)
//             delete type_chunk;
//     }

//     return type_chunks_all;
// }
// //
// // destroys a datatype
// //
// decaf::
// Datatype::~Datatype()
// {
//     MPI_Type_free(&datatype_);
// }

// #endif
