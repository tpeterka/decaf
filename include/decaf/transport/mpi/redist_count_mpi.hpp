//---------------------------------------------------------------------------
//
// data interface
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_REDIST_COUNT_MPI_HPP
#define DECAF_REDIST_COUNT_MPI_HPP

#include <iostream>
#include <assert.h>

#include <decaf/redist_comp.h>


namespace decaf
{

  // This class defines the common interface for the redistribution component (MxN)
  // This interface is independant from the datatype or the transport
  // implementation. Specialized components will implement the redistribution
  // in fonction of transport layer
  class RedistCountMPI : public RedistComp
  {
  public:
      RedistCountMPI(){}
      RedistCountMPI(int rankSource, int nbSources,
                     int rankDest, int nbDests,
                    CommHandle communicator);

      void put(std::shared_ptr<BaseData> data, TaskType task_type);
      void get(std::shared_ptr<BaseData> data, TaskType task_type);

  protected:
      // Compute the values necessary to determine how the data should be splitted
      // and redistributed.
      virtual void computeGlobal(std::shared_ptr<BaseData> data);

      // Seperate the Data into chunks for each destination involve in the component
      // and fill the splitChunks vector
      virtual void splitData(std::shared_ptr<BaseData> data);

      // Transfert the chunks from the sources to the destination. The data should be
      // be stored in the vector receivedChunks
      virtual void redistribute(std::shared_ptr<BaseData> data);

      // Merge the chunks from the vector receivedChunks into one single Data.
      virtual std::shared_ptr<BaseData> merge();

      // Merge the chunks from the vector receivedChunks into one single data->
      //virtual BaseData* Merge();

      bool isSource(){ return rank_ <  nbSources_; }
      bool isDest(){ return rank_ >= rankDest_ - rankSource_; }

      CommHandle communicator_; // Communicator for all the processes involve
      CommHandle commSources_;  // Communicator of the sources
      CommHandle commDests_;    // Communicator of the destinations
      int rank_;                // Rank in the group communicator
      int size_;                // Size of the group communicator

      // We keep these values so we can reuse them between 2 iterations
      int global_item_rank_;    // Index of the first item in the global array
      int global_nb_items_;     // Number of items in the global array



  };

} // namespace

decaf::
RedistCountMPI::RedistCountMPI(int rankSource, int nbSources,
           int rankDest, int nbDests, CommHandle world_comm) :
    RedistComp(rankSource, nbSources, rankDest, nbDests)
{

    MPI_Group group, groupRedist, groupSource, groupDest;
    int range[3];
    MPI_Comm_group(world_comm, &group);
    int world_rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


    //Generation of the group with all the sources and destination
    range[0] = rankSource;
    range[1] = max(rankSource + nbSources - 1, rankDest + nbDests - 1);
    range[2] = 1;

    MPI_Group_range_incl(group, 1, &range, &groupRedist);
    MPI_Comm_create_group(world_comm, groupRedist, 0, &communicator_);
    MPI_Comm_rank(communicator_, &rank_);
    MPI_Comm_size(communicator_, &size_);
    std::cout<<"Rank in the Redist component : "<<rank_<<std::endl;

    //Generation of the group with all the sources
    if(world_rank >= rankSource_ && world_rank < rankSource_ + nbSources_)
    {
        range[0] = rankSource;
        range[1] = rankSource + nbSources - 1;
        range[2] = 1;
        MPI_Group_range_incl(group, 1, &range, &groupSource);
        MPI_Comm_create_group(world_comm, groupSource, 0, &commSources_);
        MPI_Group_free(&groupSource);
        int source_rank;
        MPI_Comm_rank(commSources_, &source_rank);
        std::cout<<"Source Rank in the Redist component : "<<source_rank<<std::endl;
    }

    //Generation of the group with all the Destinations
    if(world_rank >= rankDest_ && world_rank < rankDest_ + nbDests_)
    {
        range[0] = rankDest;
        range[1] = rankDest + nbDests - 1;
        range[2] = 1;
        MPI_Group_range_incl(group, 1, &range, &groupDest);
        MPI_Comm_create_group(world_comm, groupDest, 0, &commDests_);
        MPI_Group_free(&groupDest);
        int dest_rank;
        MPI_Comm_rank(commDests_, &dest_rank);
        std::cout<<"Dest Rank in the Redist component : "<<dest_rank<<std::endl;

    }

    MPI_Group_free(&group);
    MPI_Group_free(&groupRedist);

}

void
decaf::
RedistCountMPI::put(std::shared_ptr<BaseData> data, TaskType task_type)
{
    computeGlobal(data);

    splitData(data);

    redistribute(data);


}

void
decaf::
RedistCountMPI::computeGlobal(std::shared_ptr<BaseData> data)
{
    if(isSource())
    {
        int nbItems = data->getNbItems();
        std::cout<<"Nombre d'Item local : "<<nbItems<<std::endl;

        //Computing the index of the local first item in the global array of data
        MPI_Scan(&nbItems, &global_item_rank_, 1, MPI_INT,
                 MPI_SUM, commSources_);
        global_item_rank_ -= nbItems;   // Process rank 0 has the item 0,
                                        // rank 1 has the item nbItems(rank 0)
                                        // and so on
        std::cout<<"Local index of first item : "<<global_item_rank_<<std::endl;

        //Compute the number of items in the global array
        MPI_Allreduce(&nbItems, &global_nb_items_, 1, MPI_INT,
                      MPI_SUM, commSources_);

        std::cout<<"Total number of items : "<<global_nb_items_<<std::endl;
    }
    std::cout<<"Redistributing with the rank "<<rank_<<std::endl;
    std::cout<<"Rank source : "<<rankSource_<<", Rank Destination : "<<rankDest_<<std::endl;
    if(isSource())
        std::cout<<"I'm a source in the redistribution"<<std::endl;
    if(isDest())
        std::cout<<"I'm a dest in the redistribution"<<std::endl;

}

void
decaf::
RedistCountMPI::splitData(shared_ptr<BaseData> data)
{
    std::cout<<"Spliting the data"<<std::endl;
    std::cout<<"Redistributing with the rank "<<rank_<<std::endl;
    if(isSource()){
        // We have the items global_item_rank to global_item_rank_ + data->getNbItems()
        // We have to split global_nb_items_ into nbDest in total

        //Computing the number of elements to split
        int items_per_dest = global_nb_items_ /  nbDests_;

        int rankOffset = global_nb_items_ %  nbDests_;

        //debug
        //std::cout<<"Ranks from 0 to "<<rankOffset-1<<" will receive "<< items_per_dest+1 <<
        //           " items. Ranks from "<<rankOffset<< " to "<<  nbDests_-1<<
        //           " will reiceive" <<items_per_dest<< " items"<< std::endl;

        //Computing how to split the data

        //Compute the destination rank of the first item
        int first_rank;
        if( rankOffset == 0){ //  Case where nbDest divide the total number of item
            first_rank = global_item_rank_ / items_per_dest;
        }
        else
        {
            // The first ranks have items_per_dest+1 items
            first_rank = global_item_rank_ / (items_per_dest+1);

            //If we starts
            if(first_rank >= rankOffset)
            {
                first_rank = rankOffset +
                        (global_item_rank_ - rankOffset*(items_per_dest+1)) / items_per_dest;
            }
        }
        std::cout<<"First rank to send data : "<<first_rank<<std::endl;

        //Compute the split vector and the destination ranks
        std::vector<int> split_ranges;
        int items_left = data->getNbItems();
        int current_rank = first_rank;

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete  summerizeDest_;
         summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        while(items_left != 0)
        {
            int currentNbItems;
            //We may have to complete the rank
            if(current_rank == first_rank){
                int global_item_firstrank;
                if(first_rank < rankOffset)
                {
                    global_item_firstrank = first_rank * (items_per_dest + 1);
                    currentNbItems = min(items_left,
                                         global_item_firstrank + items_per_dest + 1 - global_item_rank_);
                }
                else
                {
                    global_item_firstrank = rankOffset * (items_per_dest + 1) +
                            (first_rank - rankOffset) * items_per_dest ;
                    currentNbItems = min(items_left,
                                     global_item_firstrank + items_per_dest - global_item_rank_);

                }


            }
            else if(current_rank < rankOffset)
                currentNbItems = min(items_left, items_per_dest + 1);
            else
                currentNbItems = min(items_left, items_per_dest);

            split_ranges.push_back(currentNbItems);
             summerizeDest_[current_rank] = 1;
            // rankDest_ - rankSource_ is the rank of the first destination in the
            // component communicator (communicator_)
             destList_.push_back(current_rank + ( rankDest_ -  rankSource_));
            items_left -= currentNbItems;
            current_rank++;
        }
        std::cout<<"Data will be split in "<<split_ranges.size()<<"chunks"<<std::endl;

        splitChunks_ =  data->split( split_ranges );
        std::cout<<splitChunks_.size()<<" chunks have been produced"<<std::endl;

        std::cout<<"Serializing the chunks..."<<std::endl;
        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            if(!splitChunks_.at(i)->serialize())
                std::cout<<"ERROR : unable to serialize one object"<<std::endl;
        }
    }
    else
        std::cout<<"Destination, nothing to do in the split"<<std::endl;

}

void
decaf::
RedistCountMPI::redistribute(std::shared_ptr<BaseData> data)
{
    std::cout<<"Redistributing with the rank "<<rank_<<std::endl;
    // Sum the number of emission for each destination
    int *sum;
    if(isSource())
    {
        if(rank_ == 0) sum = new int[ nbDests_];
        MPI_Reduce( summerizeDest_, sum,  nbDests_, MPI_INT, MPI_SUM,
                   0, commSources_);
    }

    // Sending to the rank 0 of the destinations
    if(rank_ == 0)
    {
        for(int i = 0; i < nbDests_;i++)
            std::cout<<"Destination "<<i<<" : "<<sum[i]<<std::endl;
        MPI_Send(sum,  nbDests_, MPI_INT,  rankDest_ -  rankSource_, 0, communicator_);
    }



    // Getting the accumulated buffer on the destination side
    int *destBuffer;
    if(rank_ ==  rankDest_ -  rankSource_) //Root of destination
    {
        destBuffer = new int[ nbDests_];
        MPI_Recv(destBuffer,  nbDests_, MPI_INT, 0, 0, communicator_, MPI_STATUS_IGNORE);
        for(int i = 0; i < nbDests_;i++)
            std::cout<<"Reception : Destination "<<i<<" : "<<destBuffer[i]<<std::endl;
    }

    // Scattering the sum accross all the destinations
    int nbRecep;
    if(isDest())
    {
        std::cout<<"Scaterring with the rank "<<rank_<<std::endl;
        MPI_Scatter(destBuffer,  1, MPI_INT, &nbRecep, 1, MPI_INT, 0, commDests_);

        if(rank_ ==  rankDest_ -  rankSource_) delete destBuffer;
    }

    // At this point, each source knows where they have to send data
    // and each destination knows how many message it should receive

    //Processing the data exchange
    if(isSource())
    {
        for(unsigned int i = 0; i <  destList_.size(); i++)
        {
            MPI_Send( splitChunks_.at(i)->getSerialBuffer(),
                      splitChunks_.at(i)->getSerialBufferSize(),
                      MPI_BYTE, destList_.at(i), 0, communicator_);
        }
    }

    if(isDest())
    {
        for(int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, communicator_, &status);
            if (status.MPI_TAG == 0)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                //Allocating the space necessary
                //std::shared_ptr<char> serialbuffer (new char[nitems], std::default_delete<char[]>());
                data->allocate_serial_buffer(nitems);
                //std::vector<char> buffer(nitems);

                MPI_Recv(data->getSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                         status.MPI_TAG, communicator_, &status);

                data->merge();
                //A modifier
                //shared_ptr<BaseData> newData = make_shared<BaseData>(data->get());
                //receivedChunks_.push_back(newData);
            }

        }
    }


}
// Merge the chunks from the vector receivedChunks into one single Data.
std::shared_ptr<decaf::BaseData>
decaf::
RedistCountMPI::merge()
{
    return std::shared_ptr<BaseData>();
}

/*decaf::BaseData* decaf::RedistCountMPI::Merge(BaseData*)
{
    if( isDest())
    {
        if( receivedChunks_.empty()) return NULL;
        if( receivedChunks_.size() == 1) return  receivedChunks_.front();

        BaseData *mergeData =  receivedChunks_.at(0);
        for(int i = 1; i <  receivedChunks_.size(); i++){
            mergeData->merge( receivedChunks_.at(i));
            delete  receivedChunks_.at(i);   // The necessary data are copied inside the
                                            // other object, we can safely clean
        }

        return mergeData;
    }

}*/



#endif
