#include <bredala/transport/split.h>
#include <sstream>

bool
decaf::split_by_count(pConstructData& data,                             // Data model to split
                            RedistRole role,                            // Role in the redistribution
                            int global_nb_items,                        // Total number of items to split
                            int global_item_rank,                       // Global rank of the first item of the data model
                            std::vector<pConstructData>& splitChunks,   // Data models produced by the split. Should match the number of destinations
                            std::vector<pConstructData>& splitBuffer,   // Data models pre allocated.
                            int nbDests,                                // Number of destinations = number of sub data models to produce
                            int local_dest_rank,
                            int rank,
                            int* summerizeDest,                         // Size of nb_Dest. 1 if a message should be send, 0 if the message is empty
                            std::vector<int>& destList,                 // Size of nb_Dest. an ID rank if a message should be sent, -1 if the message is empty
                            RedistCommMethod commMethod)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        //if( summerizeDest_) delete [] summerizeDest_;
        //summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest,  nbDests * sizeof(int)); // First we don't send anything

        // Clearing the send buffer from previous iteration
        splitChunks.clear();
        destList.clear();

        // Case where the current data model does not have data
        if(!data.getPtr() || data->getNbItems() == 0)
        {
            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data.getPtr() || !data->hasSystem())
            {
                if(commMethod == DECAF_REDIST_P2P)
                {
                    while(destList.size() < nbDests)
                    {
                        destList.push_back(-1);
                        splitChunks.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = 0;
                while(destList.size() < nbDests)
                {
                    destList.push_back(local_dest_rank + dest_rank);
                    splitChunks.push_back(pConstructData());
                    splitChunks.back()->copySystemFields(data);
                    splitChunks.back()->serialize();

                    if( commMethod == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank != rank)
                            summerizeDest[dest_rank] = 1;
                    dest_rank++;
                }
            }
        }
        else
        {


            // We have the items global_item_rank to global_item_rank_ + data->getNbItems()
            // We have to split global_nb_items_ into nbDest in total

            //Computing the number of elements to split
            int items_per_dest = global_nb_items /  nbDests;

            int rankOffset = global_nb_items %  nbDests;

            //Computing how to split the data

            //Compute the destination rank of the first item
            int first_rank;
            if(items_per_dest > 0) // More items than number of destination
            {
                if( rankOffset == 0) //  Case where nbDest divide the total number of item
                {
                    first_rank = global_item_rank / items_per_dest;
                }
                else
                {
                    // The first ranks have items_per_dest+1 items
                    first_rank = global_item_rank / (items_per_dest+1);

                    //If we starts
                    if(first_rank >= rankOffset)
                    {
                        first_rank = rankOffset +
                                (global_item_rank - rankOffset*(items_per_dest+1)) / items_per_dest;
                    }
                }
            }
            else
            {
                // If there are less items then destination,
                // only global_nb_items destinations will receive 1 item
                first_rank = global_item_rank;
            }

            //fprintf(stderr, "Global_item_rank: %i, first_rank: %i\n", global_item_rank, first_rank);


            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data->hasSystem())
            {
                if(commMethod == DECAF_REDIST_P2P)
                {
                    while(destList.size() < first_rank)
                    {
                        destList.push_back(-1);
                        splitChunks.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = 0;
                while(destList.size() < first_rank)
                {
                    destList.push_back(local_dest_rank + dest_rank);
                    splitChunks.push_back(pConstructData());
                    splitChunks.back()->copySystemFields(data);
                    splitChunks.back()->serialize();

                    if( commMethod == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank != rank)
                            summerizeDest[dest_rank] = 1;
                    dest_rank++;
                }
            }

            //Compute the split vector and the destination ranks
            std::vector<int> split_ranges;
            int items_left = data->getNbItems();
            int current_rank = first_rank;

            unsigned int nbChunks = 0;
            while(items_left != 0)
            {
                int currentNbItems;
                //We may have to complete the rank
                if(current_rank == first_rank){
                    int global_item_firstrank;
                    if(first_rank < rankOffset)
                    {
                        global_item_firstrank = first_rank * (items_per_dest + 1);
                        currentNbItems = std::min(items_left,
                                             global_item_firstrank + items_per_dest + 1 - global_item_rank);
                    }
                    else
                    {
                        global_item_firstrank = rankOffset * (items_per_dest + 1) +
                                (first_rank - rankOffset) * items_per_dest ;
                        currentNbItems = std::min(items_left,
                                         global_item_firstrank + items_per_dest - global_item_rank);

                    }


                }
                else if(current_rank < rankOffset)
                    currentNbItems = std::min(items_left, items_per_dest + 1);
                else
                    currentNbItems = std::min(items_left, items_per_dest);

                split_ranges.push_back(currentNbItems);

                //We won't send a message if we send to self
                if(current_rank + local_dest_rank != rank)
                    summerizeDest[current_rank] = 1;
                // rankDest_ - rankSource_ is the rank of the first destination in the
                // component communicator (communicator_)

                destList.push_back(current_rank + local_dest_rank);
                items_left -= currentNbItems;
                current_rank++;
                nbChunks++;
            }

            // Shaping the buffers, ie the results of the split
            if(splitBuffer.empty() && nbChunks > 0)
                // We prealloc with 0 to avoid allocating too much iterations
                // The first iteration will make a reasonable allocation
                data.preallocMultiple(nbChunks, 0, splitBuffer);
            else if(nbChunks > 0)
            {
                // If we have more buffer than needed, the remove the excedent
                if(splitBuffer.size() > nbChunks)
                    splitBuffer.resize(nbChunks);

                for(unsigned int i = 0; i < splitBuffer.size(); i++)
                    splitBuffer[i]->softClean();

                // If we don't have enough buffers, we complete it
                if(nbChunks - splitBuffer.size() > 0)
                {
                    std::vector< pConstructData > newBuffers;
                    data.preallocMultiple(nbChunks - splitBuffer.size(), 0, newBuffers);
                    splitBuffer.insert(splitBuffer.end(), newBuffers.begin(), newBuffers.end());
                }

            }

            //std::stringstream ss;
            //ss<<"Range :[";
            //for (unsigned int i = 0; i < split_ranges.size(); i++)
            //    ss<<split_ranges[i]<<",";
            //ss<<"]";
            //fprintf(stderr, "%s\n", ss.str().c_str());

            data->split(split_ranges, splitBuffer);

            //std::stringstream ss1;
            //ss1<<"Generated :[";
            //for (unsigned int i = 0; i < split_ranges.size(); i++)
            //    ss1<<splitBuffer[i]->getNbItems()<<",";
            //ss1<<"]";
            //fprintf(stderr, "%s\n", ss1.str().c_str());

            for(unsigned int i = 0; i < splitBuffer.size(); i++)
                splitChunks.push_back(splitBuffer[i]);

            for(unsigned int i = 0; i < splitChunks.size(); i++)
            {
                // TODO : Check the rank for the destination.
                // Not necessary to serialize if overlapping
                if(!splitChunks[i].empty() && !splitChunks[i]->serialize())
                    std::cout<<"ERROR : unable to serialize one object"<<std::endl;
            }

            //fprintf(stderr, "Size of chunks after append of buffers: %lu\n", splitChunks.size());

            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data->hasSystem())
            {
                if(commMethod == DECAF_REDIST_P2P)
                {
                    while(destList.size() < nbDests)
                    {
                        destList.push_back(-1);
                        splitChunks.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = current_rank;
                while(destList.size() < nbDests)
                {
                    destList.push_back(local_dest_rank + dest_rank);
                    splitChunks.push_back(pConstructData());
                    splitChunks.back()->copySystemFields(data);
                    splitChunks.back()->serialize();

                    if( commMethod == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank != rank)
                            summerizeDest[dest_rank] = 1;
                    dest_rank++;
                }
            }

            // DEPRECATED
            // Everything is done, now we can clean the data.
            // Data might be rewriten if producers and consummers are overlapping

            // data->purgeData();

            //std::stringstream ss2;
            //ss2<<"Chunks :[";
            //for (unsigned int i = 0; i < splitChunks.size(); i++)
            //{
            //    if(splitChunks[i].getPtr())
            //        ss2<<splitChunks[i]->getNbItems()<<",";
            //    else
            //        ss2<<"0,";
            //}
            //ss2<<"]";
            //fprintf(stderr, "%s\n", ss2.str().c_str());

        }
    }

    return true;
}

bool
decaf::split_by_round(pConstructData& data,                             // Data model to split
                           RedistRole role,                             // Role in the redistribution
                           int global_item_rank,                        // Global rank of the first item of the data model
                           std::vector<pConstructData>& splitChunks,    // Data models produced by the split. Should match the number of destinations
                           std::vector<pConstructData>& splitBuffer,    // Data models pre allocated.
                           int nbDests,                                 // Number of destinations = number of sub data models to produce
                           int local_dest_rank,                         // Rank Id of the first destination rank in the global communicator
                           int rank,                                    // Rank Id of the current process
                           int* summerizeDest,                          // Size of nb_Dest. 1 if a message should be send, 0 if the message is empty
                           std::vector<int>& destList,                  // Size of nb_Dest. an ID rank if a message should be sent, -1 if the message is empty
                           RedistCommMethod commMethod)                 // Redistribution method (collective, p2p)
{
    if(role == DECAF_REDIST_SOURCE)
    {

        //Computing how to split the data

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        //if( summerizeDest_) delete [] summerizeDest_;
        //summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest,  nbDests * sizeof(int)); // First we don't send anything

        // Clearing the send buffer from previous iteration
        splitChunks.clear();
        destList.clear();

        // Case where the current data model does not have data
        if(!data.getPtr() || data->getNbItems() == 0)
        {
            // For P2P, destList must have the same size as the number of destination.
            // We fill the destinations with no messages with -1 (= send empty message)
            if(!data.getPtr() || !data->hasSystem())
            {
                if(commMethod == DECAF_REDIST_P2P)
                {
                    while(destList.size() < nbDests)
                    {
                        destList.push_back(-1);
                        splitChunks.push_back(pConstructData(false));
                    }
                }
            }
            else    // If the data has system fields, we need to forward them to every destination
            {
                int dest_rank = 0;
                while(destList.size() < nbDests)
                {
                    destList.push_back(local_dest_rank + dest_rank);
                    splitChunks.push_back(pConstructData());
                    splitChunks.back()->copySystemFields(data);
                    splitChunks.back()->serialize();

                    if( commMethod == DECAF_REDIST_COLLECTIVE &&
                        dest_rank + local_dest_rank != rank)
                            summerizeDest[dest_rank] = 1;
                    dest_rank++;
                }
            }
        }
        else
        {

            //Compute the split vector and the destination ranks
            std::vector<std::vector<int> > split_ranges = std::vector<std::vector<int> >( nbDests);
            int nbItems = data->getNbItems();

            //Distributing the data in a round robin fashion
            for(int i = 0; i < nbItems; i++)
            {
                split_ranges[(global_item_rank + i) % nbDests].push_back(i);
                split_ranges[(global_item_rank + i) % nbDests].push_back(1);
            }

            // Add the total of items for 1 destination.
            // Each interval has 1 item so the form is always [[offset1,1][offset2,1]...]
            // The total number of items for 1 destination is then size() / 2
            for(unsigned int i = 0; i < split_ranges.size(); i++)
                split_ranges[i].push_back(split_ranges[i].size() / 2);

            //Updating the informations about messages to send

            for(unsigned int i = 0; i < split_ranges.size(); i++)
            {
                if(split_ranges[i].size() > 0)
                {
                    destList.push_back(i + local_dest_rank);

                    //We won't send a message if we send to self
                    if(i + local_dest_rank != rank)
                        summerizeDest[i] = 1;
                }
                else
                    destList.push_back(-1);

                // We don't need a special case for P2P because
                // we create already as many sub data models as
                // destinations

            }

            if(splitBuffer.empty())
                // We prealloc with 0 to avoid allocating too much iterations
                // The first iteration will make a reasonable allocation
                data.preallocMultiple(nbDests, 0, splitBuffer);
            else
            {
                // No need to adjust the number of buffer, always equal to the number of destination
                for(unsigned int i = 0; i < splitBuffer.size(); i++)
                    splitBuffer[i]->softClean();
            }

            data->split(split_ranges, splitBuffer);

            for(unsigned int i = 0; i < splitBuffer.size(); i++)
            {
                splitChunks.push_back(splitBuffer[i]);
                if(data->hasSystem())
                    splitChunks.back()->copySystemFields(data);
            }

            for(unsigned int i = 0; i < splitChunks.size(); i++)
            {
                // TODO : Check the rank for the destination.
                // Not necessary to serialize if overlapping
                if(!splitChunks[i]->serialize())
                    std::cout<<"ERROR : unable to serialize one object"<<std::endl;
            }

            // DEPRECATED
            // Everything is done, now we can clean the data.
            // Data might be rewriten if producers and consummers are overlapping

            // data->purgeData();
        }

    }

    return true;
}

