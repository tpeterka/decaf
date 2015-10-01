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

#include <iostream>
#include <assert.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>

#include <decaf/transport/mpi/redist_block_mpi.h>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/blockconstructdata.hpp>

void printExtends(std::vector<unsigned int>& extend)
{
    std::cout<<"["<<extend[0]<<","<<extend[1]<<","<<extend[2]<<"]"
             <<"["<<extend[0]+extend[3]<<","<<extend[1]+extend[4]<<","<<extend[2]+extend[5]<<"]"<<std::endl;
}


namespace decaf
{

bool
decaf::
RedistBlockMPI::isSource()
{
    return rank_ >= local_source_rank_ && rank_ < local_source_rank_ + nbSources_;
}

bool
decaf::
RedistBlockMPI::isDest()
{
    return rank_ >= local_dest_rank_ && rank_ < local_dest_rank_ + nbDests_;
}

decaf::
RedistBlockMPI::RedistBlockMPI(int rankSource, int nbSources,
                               int rankDest, int nbDests, CommHandle world_comm) :
    RedistComp(rankSource, nbSources, rankDest, nbDests),
    communicator_(MPI_COMM_NULL),
    commSources_(MPI_COMM_NULL),
    commDests_(MPI_COMM_NULL)
{
    MPI_Group group, groupRedist, groupSource, groupDest;
    int range[3];
    MPI_Comm_group(world_comm, &group);
    int world_rank, world_size;

    MPI_Comm_rank(world_comm, &world_rank);
    MPI_Comm_size(world_comm, &world_size);

    local_source_rank_ = 0;                     //Rank of first source in communicator_
    local_dest_rank_ = rankDest_ - rankSource_; //Rank of first destination in communucator_

    //Generation of the group with all the sources and destination
    range[0] = rankSource;
    range[1] = std::max(rankSource + nbSources - 1, rankDest + nbDests - 1);
    range[2] = 1;

    MPI_Group_range_incl(group, 1, &range, &groupRedist);
    MPI_Comm_create_group(world_comm, groupRedist, 0, &communicator_);
    MPI_Comm_rank(communicator_, &rank_);
    MPI_Comm_size(communicator_, &size_);

    //Generation of the group with all the sources
    //if(world_rank >= rankSource_ && world_rank < rankSource_ + nbSources_)
    if(isSource())
    {
        range[0] = rankSource;
        range[1] = rankSource + nbSources - 1;
        range[2] = 1;
        MPI_Group_range_incl(group, 1, &range, &groupSource);
        MPI_Comm_create_group(world_comm, groupSource, 0, &commSources_);
        MPI_Group_free(&groupSource);
        int source_rank;
        MPI_Comm_rank(commSources_, &source_rank);
    }

    //Generation of the group with all the Destinations
    //if(world_rank >= rankDest_ && world_rank < rankDest_ + nbDests_)
    if(isDest())
    {
        range[0] = rankDest;
        range[1] = rankDest + nbDests - 1;
        range[2] = 1;
        MPI_Group_range_incl(group, 1, &range, &groupDest);
        MPI_Comm_create_group(world_comm, groupDest, 0, &commDests_);
        MPI_Group_free(&groupDest);
        int dest_rank;
        MPI_Comm_rank(commDests_, &dest_rank);
    }

    MPI_Group_free(&group);
    MPI_Group_free(&groupRedist);

    destBuffer_ = new int[nbDests_];
    sum_ = new int[nbDests_];
}

decaf::
RedistBlockMPI::~RedistBlockMPI()
{
  if (communicator_ != MPI_COMM_NULL)
    MPI_Comm_free(&communicator_);
  if (commSources_ != MPI_COMM_NULL)
    MPI_Comm_free(&commSources_);
  if (commDests_ != MPI_COMM_NULL)
    MPI_Comm_free(&commDests_);

  delete[] destBuffer_;
  delete[] sum_;
}

/*void
decaf::
RedistBlockMPI::splitBlock(Block<3> & base, int nbSubblock)
{
    // Naive version, we split everything in the same dimension
    if(!base.hasGlobalExtends_)
    {
        std::cerr<<"ERROR : The global domain doesn't have global extends. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if(!base.hasGlobalBBox_)
    {
        std::cerr<<"ERROR : The global domain doesn't have global bbox. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if(!base.hasGridspace_)
    {
        std::cerr<<"ERROR : The global domain doesn't have gridspace. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if(base.globalExtends_[3] < nbSubblock && base.globalExtends_[4] && base.globalExtends_[5])
    {
        std::cerr<<"ERROR : none of the dimension is large enough to cut in "<<nbSubblock<<" parts. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    // Checking which dimension we are using
    int dim = 0;
    while(base.globalExtends_[3+dim] < nbSubblock)
        dim++;

    std::cout<<"Using the dimension "<<dim<<std::endl;


    // Building the subblocks
    unsigned int extendOffset = 0;
    for(int i = 0; i < nbSubblock; i++)
    {
        Block<3> sub;
        sub.setGridspace(base.gridspace_);
        sub.setGlobalBBox(base.globalBBox_);
        sub.setGlobalExtends(base.globalExtends_);

        std::vector<unsigned int> localextends = base.globalExtends_;
        localextends[dim] = extendOffset;
        if(i < base.globalExtends_[3+dim] % nbSubblock)
        {
            localextends[3+dim] = (base.globalExtends_[3+dim] / nbSubblock) + 1;
            extendOffset += (base.globalExtends_[3+dim] / nbSubblock) + 1;
        }
        else
        {
            localextends[3+dim] = base.globalExtends_[3+dim] / nbSubblock;
            extendOffset += base.globalExtends_[3+dim] / nbSubblock;
        }

        std::vector<float> localbox;
        for(int j = 0; j < 6; j++)
            localbox.push_back((float)localextends[j] * sub.gridspace_);

        sub.setLocalBBox(localbox);
        sub.setLocalExtends(localextends);

        subblocks_.push_back(sub);
    }

}*/

// TODO : remove the recursion on this
void splitExtends(std::vector<unsigned int>& extends, int nbSubblock, std::vector< std::vector<unsigned int> >& subblocks)
{
    if(nbSubblock == 1)
    {
        subblocks.push_back(extends);
        return ;
    }

    //Determine the highest dimension
    int dim = 0;
    if(extends[4] > extends[3+dim])
        dim = 1;
    if(extends[5] > extends[3+dim])
        dim = 2;

    assert(extends[dim+3] >= 2);

    //For this block, only the dimension in which we cut is changed
    std::vector<unsigned int> block1(extends);
    block1[3+dim] = (extends[3+dim] / 2) + extends[3+dim] % 2;

    std::vector<unsigned int> block2(extends);
    block2[dim] += block1[3+dim];
    block2[3+dim] = (extends[3+dim] / 2);

    if(nbSubblock == 2)
    {
        subblocks.push_back(block1);
        subblocks.push_back(block2);
        return;
    }
    else
    {
        splitExtends(block1, (nbSubblock / 2) + nbSubblock % 2, subblocks);
        splitExtends(block2, (nbSubblock / 2), subblocks);
    }

    return;
}

void
decaf::
RedistBlockMPI::splitBlock(Block<3> & base, int nbSubblock)
{

    // BVH spliting method : we cut on the highest dimension
    if(!base.hasGlobalExtends_)
    {
        std::cerr<<"ERROR : The global domain doesn't have global extends. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if(!base.hasGlobalBBox_)
    {
        std::cerr<<"ERROR : The global domain doesn't have global bbox. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if(!base.hasGridspace_)
    {
        std::cerr<<"ERROR : The global domain doesn't have gridspace. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    if(base.globalExtends_[3] < nbSubblock && base.globalExtends_[4] && base.globalExtends_[5])
    {
        std::cerr<<"ERROR : none of the dimension is large enough to cut in "<<nbSubblock<<" parts. Abording."<<std::endl;
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    std::vector< std::vector<unsigned int> > subBlocks;
    splitExtends(base.globalExtends_, nbSubblock, subBlocks);

    //Generating the corresponding BBox
    for(unsigned int i = 0; i < subBlocks.size(); i++)
    {

        Block<3> newBlock;
        newBlock.setGridspace(base.gridspace_);
        newBlock.setGlobalBBox(base.globalBBox_);
        newBlock.setGlobalExtends(base.globalExtends_);
        newBlock.setLocalExtends(subBlocks.at(i));
        std::vector<float> localBBox;
        for(unsigned int j = 0; j < 6; j++)
            localBBox.push_back((float)subBlocks.at(i)[j] * base.gridspace_);
        newBlock.setLocalBBox(localBBox);

        if(base.hasGhostRegions())
            newBlock.buildGhostRegions(base.ghostSize_);

        subblocks_.push_back(newBlock);
    }
}


void
decaf::
RedistBlockMPI::updateBlockDomainFields()
{
    assert(subblocks_.size() == splitChunks_.size());

    for(unsigned int i = 0; i < subblocks_.size(); i++)
    {
        std::shared_ptr<BlockConstructData> newBlock = std::make_shared<BlockConstructData>(subblocks_.at(i));
        std::shared_ptr<ConstructData> container = std::dynamic_pointer_cast<ConstructData>(splitChunks_.at(i));
        container->updateData("domain_block", newBlock);
    }
}

void
decaf::
RedistBlockMPI::computeGlobal(std::shared_ptr<BaseData> data, RedistRole role)
{

    // The user has to provide the global block structure.
    // Checking we have all the informations

    if(role == DECAF_REDIST_SOURCE)
    {

        std::shared_ptr<ConstructData> container = std::dynamic_pointer_cast<ConstructData>(data);

        if(!container)
        {
            std::cerr<<"ERROR : Can not convert the data into a ConstructData. ConstructData "
                     <<"is required when using the Redist_block_mpi redistribution."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        std::shared_ptr<BlockConstructData> blockData =
                container->getTypedData<BlockConstructData>("domain_block");
        if(!blockData)
        {
            std::cerr<<"ERROR : Can not access or convert the field \'domain_block\' in the data model. "
                     <<"Abording."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        // Building the sub blocks and pushing them in subblocks_
        splitBlock(blockData->getData(), nbDests_);
    }
}

void
decaf::
RedistBlockMPI::splitData(std::shared_ptr<BaseData> data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE){

        std::shared_ptr<ConstructData> container = std::dynamic_pointer_cast<ConstructData>(data);

        if(!container)
        {
            std::cerr<<"ERROR : Can not convert the data into a ConstructData. ConstructData "
                     <<"is required when using the Redist_block_mpi redistribution."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete  summerizeDest_;
         summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        splitChunks_ =  container->split( subblocks_ );

        //Pushing the subdomain block into each split chunk.
        //The field domain_block is updated
        updateBlockDomainFields();

        //Updating the informations about messages to send
        for(unsigned int i = 0; i < subblocks_.size(); i++)
        {
            if(splitChunks_.at(i)->getNbItems() > 0)
            {
                destList_.push_back(i + local_dest_rank_);

                //We won't send a message if we send to self
                if(i + local_dest_rank_ != rank_)
                    summerizeDest_[i] = 1;
            }
            else //No data for this split
                destList_.push_back(-1);
        }

        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            if(splitChunks_.at(i)->getNbItems() > 0)
            {
                // TODO : Check the rank for the destination.
                // Not necessary to serialize if overlapping
                if(!splitChunks_.at(i)->serialize())
                    std::cout<<"ERROR : unable to serialize one object"<<std::endl;
            }
        }

        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping
        data->purgeData();
    }
}

void
decaf::
RedistBlockMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{
    // Sum the number of emission for each destination
    if(role == DECAF_REDIST_SOURCE)
    {
        MPI_Reduce( summerizeDest_, sum_,  nbDests_, MPI_INT, MPI_SUM,
                   local_source_rank_, commSources_);
    }

    //Case with overlapping
    if(rank_ == local_source_rank_ && rank_ == local_dest_rank_)
    {
        memcpy(destBuffer_, sum_, nbDests_ * sizeof(int));
    }
    else
    {
        // Sending to the rank 0 of the destinations
        if(role == DECAF_REDIST_SOURCE && rank_ == local_source_rank_)
        {
            MPI_Request req;
            reqs.push_back(req);
            MPI_Isend(sum_,  nbDests_, MPI_INT,  local_dest_rank_, MPI_METADATA_TAG, communicator_,&reqs.back());
        }


        // Getting the accumulated buffer on the destination side
        if(role == DECAF_REDIST_DEST && rank_ ==  local_dest_rank_) //Root of destination
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_METADATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_METADATA_TAG)  // normal, non-null get
            {
                //destBuffer = new int[ nbDests_];
                MPI_Recv(destBuffer_,  nbDests_, MPI_INT, local_source_rank_, MPI_METADATA_TAG, communicator_, MPI_STATUS_IGNORE);
            }
        }
    }

    // Scattering the sum accross all the destinations
    int nbRecep;
    if(role == DECAF_REDIST_DEST)
    {
        MPI_Scatter(destBuffer_,  1, MPI_INT, &nbRecep, 1, MPI_INT, 0, commDests_);
    }

    // At this point, each source knows where they have to send data
    // and each destination knows how many message it should receive

    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {
        for(unsigned int i = 0; i <  destList_.size(); i++)
        {
            //Sending to self, we simply copy the string from the out to in
            if(destList_.at(i) == rank_)
            {
                transit = splitChunks_.at(i);
            }
            else if(destList_.at(i) != -1)
            {
                MPI_Request req;
                reqs.push_back(req);

                MPI_Isend( splitChunks_.at(i)->getOutSerialBuffer(),
                          splitChunks_.at(i)->getOutSerialBufferSize(),
                          MPI_BYTE, destList_.at(i), MPI_DATA_TAG, communicator_, &reqs.back());
            }
        }
    }

    if(role == DECAF_REDIST_DEST)
    {
        for(int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_DATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_DATA_TAG)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                //Allocating the space necessary
                data->allocate_serial_buffer(nitems);
                MPI_Recv(data->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                         status.MPI_TAG, communicator_, &status);

                // The dynamic type of merge is given by the user
                // NOTE : examin if it's not more efficient to receive everything
                // and then merge. Memory footprint more important but allows to
                // aggregate the allocations etc
                data->merge();
            }

        }

        // Checking if we have something in transit
        if(transit)
        {
            data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());

            //We don't need it anymore. Cleaning for the next iteration
            transit.reset();
        }
    }
}

// Merge the chunks from the vector receivedChunks into one single Data.
std::shared_ptr<decaf::BaseData>
decaf::
RedistBlockMPI::merge(RedistRole role)
{
    return std::shared_ptr<BaseData>();
}

void
decaf::
RedistBlockMPI::flush()
{
    if(reqs.size())
        MPI_Waitall(reqs.size(), &reqs[0], MPI_STATUSES_IGNORE);
    reqs.clear();

    // Cleaning the data here because synchronous send.
    // TODO :  move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

} // namespace

