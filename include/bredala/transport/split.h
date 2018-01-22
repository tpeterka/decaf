#ifndef SPLIT_H
#define SPLIT_H

#include <vector>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/blockconstructdata.hpp>
#include <bredala/transport/redist_comp.h>
#include <memory>

namespace decaf {

bool split_by_count(pConstructData& data,                        // Data model to split
                           RedistRole role,                             // Role in the redistribution
                           unsigned long long global_nb_items,          // Total number of items to split
                           unsigned long long global_item_rank,         // Global rank of the first item of the data model
                           std::vector<pConstructData>& splitChunks,    // Data models produced by the split. Should match the number of destinations
                           std::vector<pConstructData>& splitBuffer,    // Data models pre allocated.
                           int nbDests,                                 // Number of destinations = number of sub data models to produce
                           int local_dest_rank,                         // Rank Id of the first destination rank in the global communicator
                           int rank,                                    // Rank Id of the current process
                           int* summerizeDest,                          // Size of nb_Dest. 1 if a message should be send, 0 if the message is empty
                           std::vector<int>& destList,                  // Size of nb_Dest. an ID rank if a message should be sent, -1 if the message is empty
                           RedistCommMethod commMethod);                // Redistribution method (collective, p2p)


bool split_by_round(pConstructData& data,                        // Data model to split
                           RedistRole role,                             // Role in the redistribution
                           unsigned long long global_item_rank,         // Global rank of the first item of the data model
                           std::vector<pConstructData>& splitChunks,    // Data models produced by the split. Should match the number of destinations
                           std::vector<pConstructData>& splitBuffer,    // Data models pre allocated.
                           int nbDests,                                 // Number of destinations = number of sub data models to produce
                           int local_dest_rank,                         // Rank Id of the first destination rank in the global communicator
                           int rank,                                    // Rank Id of the current process
                           int* summerizeDest,                          // Size of nb_Dest. 1 if a message should be send, 0 if the message is empty
                           std::vector<int>& destList,                  // Size of nb_Dest. an ID rank if a message should be sent, -1 if the message is empty
                           RedistCommMethod commMethod);                // Redistribution method (collective, p2p)

bool split_by_domain(pConstructData& data,                              // Data model to split
                           RedistRole role,                             // Role in the redistribution
                           std::vector<pConstructData>& splitChunks,    // Data models produced by the split. Should match the number of destinations
                           std::vector<pConstructData>& splitBuffer,    // Data models pre allocated.
                           std::vector<Block<3> >& subDomains,          // Split the data by the given subdomains
                           int nbDests,                                 // Number of destinations = number of sub data models to produce
                           int local_dest_rank,                         // Rank Id of the first destination rank in the global communicator
                           int rank,                                    // Rank Id of the current process
                           int* summerizeDest,                          // Size of nb_Dest. 1 if a message should be send, 0 if the message is empty
                           std::vector<int>& destList,                  // Size of nb_Dest. an ID rank if a message should be sent, -1 if the message is empty
                           RedistCommMethod commMethod);                // Redistribution method (collective, p2p)

} //decaf
#endif
