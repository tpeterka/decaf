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
             <<"["<<extend[0]+extend[3]<<","<<extend[1]+extend[4]
             <<","<<extend[2]+extend[5]<<"]"<<std::endl;
}

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

    //Debug test
    //dim = 0;

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
        for(unsigned int j = 0; j < 3; j++)
            localBBox.push_back(newBlock.globalBBox_[j] + (float)subBlocks.at(i)[j] * base.gridspace_);
        for(unsigned int j = 3; j < 6; j++)
            localBBox.push_back((float)(subBlocks.at(i)[j]) * base.gridspace_);
        newBlock.setLocalBBox(localBBox);

        if(base.hasGhostRegions())
        {
            newBlock.buildGhostRegions(base.ghostSize_);
        }
        else
        {
            newBlock.ghostSize_ = 0;
            newBlock.setOwnExtends(newBlock.localExtends_);
            newBlock.setOwnBBox(newBlock.localBBox_);
        }

        newBlock.printExtends();
        newBlock.printBoxes();
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
            //if(splitChunks_.at(i)->getNbItems() > 0)
            //{
                // TODO : Check the rank for the destination.
                // Not necessary to serialize if overlapping
                if(!splitChunks_.at(i)->serialize())
                    std::cout<<"ERROR : unable to serialize one object"<<std::endl;
            //}
        }

        // DEPRECATED
        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping
        // data->purgeData();

        subblocks_.clear();

    }
}
