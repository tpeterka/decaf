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

#include <sys/time.h>

double timeGlobalRecep = 0;
double timeGlobalScatter = 0;
double timeGlobalReduce = 0;


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
        newBlock.setLocalExtends(subBlocks[i]);
        std::vector<float> localBBox;
        for(unsigned int j = 0; j < 3; j++)
            localBBox.push_back(newBlock.globalBBox_[j] + (float)subBlocks[i][j] * base.gridspace_);
        for(unsigned int j = 3; j < 6; j++)
            localBBox.push_back((float)(subBlocks[i][j]) * base.gridspace_);
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

        //newBlock.printExtends();
        //newBlock.printBoxes();
        subblocks_.push_back(newBlock);
    }
}


void
decaf::
RedistBlockMPI::updateBlockDomainFields()
{
    assert(subblocks_.size() == splitBuffer_.size());

    for(unsigned int i = 0; i < subblocks_.size(); i++)
    {
        std::shared_ptr<BlockConstructData> newBlock = std::make_shared<BlockConstructData>(subblocks_[i]);
        std::shared_ptr<ConstructData> container = std::dynamic_pointer_cast<ConstructData>(splitBuffer_[i]);
        container->updateData("domain_block", newBlock);
    }
}

void
decaf::
RedistBlockMPI::computeGlobal(std::shared_ptr<BaseData> data, RedistRole role)
{

    // The user has to provide the global block structure.
    // Checking we have all the informations. We only compute the blocks once.

    if(role == DECAF_REDIST_SOURCE && subblocks_.empty())
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

    double computeSplit = 0.0, computeSerialize = 0.0;
    struct timeval begin;
    struct timeval end;
    
    if(role == DECAF_REDIST_SOURCE){
        gettimeofday(&begin, NULL);

        std::shared_ptr<ConstructData> container = std::dynamic_pointer_cast<ConstructData>(data);

        if(!container)
        {
            std::cerr<<"ERROR : Can not convert the data into a ConstructData. ConstructData "
                    <<"is required when using the Redist_block_mpi redistribution."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        if(splitBuffer_.empty())
            container->preallocMultiple(nbDests_, 34000, splitBuffer_); // TODO : remove the hard coded value
        else
        {
            for(unsigned int i = 0; i < splitBuffer_.size(); i++)
                splitBuffer_[i]->softClean();
        }

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete  summerizeDest_;
        summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything


        container->split( subblocks_, splitBuffer_ );

        //Pushing the subdomain block into each split chunk.
        //The field domain_block is updated
        updateBlockDomainFields();

        //Updating the informations about messages to send
        for(unsigned int i = 0; i < subblocks_.size(); i++)
        {
            if(splitBuffer_[i]->getNbItems() > 0)
            {
                destList_.push_back(i + local_dest_rank_);

                //We won't send a message if we send to self
                if(i + local_dest_rank_ != rank_)
                    summerizeDest_[i] = 1;
            }
            else //No data for this split
                destList_.push_back(-1);
        }
        gettimeofday(&end, NULL);
        timeGlobalSplit += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

        gettimeofday(&begin, NULL);
        //std::cout<<"[";
        for(unsigned int i = 0; i < splitBuffer_.size(); i++)
        {
            //int nbItems = splitBuffer_[i]->getNbItems();
            //std::cout<<splitBuffer_[i]->getNbItems()<<",";
            if(splitBuffer_[i]->getNbItems() > 0)
            {
                // TODO : Check the rank for the destination.
                // Not necessary to serialize if overlapping
                if(!splitBuffer_[i]->serialize())
                    std::cout<<"ERROR : unable to serialize one object"<<std::endl;
            }
        }
        //std::cout<<"]"<<std::endl;
        gettimeofday(&end, NULL);
        timeGlobalBuild += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
        //if(role == DECAF_REDIST_SOURCE)
        //    printf("Compute split : %f, compute serialize : %f\n", computeSplit, computeSerialize);

        // DEPRECATED
        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping
        // data->purgeData();

        //subblocks_.clear();

    }
}

/*void
decaf::
RedistBlockMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{
    // Sum the number of emission for each destination
    if(role == DECAF_REDIST_SOURCE)
    {
	struct timeval begin;
        struct timeval end;

        gettimeofday(&begin, NULL);
        MPI_Reduce( summerizeDest_, sum_,  nbDests_, MPI_INT, MPI_SUM,
                   local_source_rank_, commSources_);
	gettimeofday(&end, NULL);
        timeGlobalReduce += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
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
	struct timeval begin;
        struct timeval end;

        gettimeofday(&begin, NULL);
        MPI_Scatter(destBuffer_,  1, MPI_INT, &nbRecep, 1, MPI_INT, 0, commDests_);
	gettimeofday(&end, NULL);
        timeGlobalScatter += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
    }

    // At this point, each source knows where they have to send data
    // and each destination knows how many message it should receive

    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {
        for(unsigned int i = 0; i <  destList_.size(); i++)
        {
            //Sending to self, we simply copy the string from the out to in
            if(destList_[i] == rank_)
            {
                transit = splitBuffer_[i];
            }
            else if(destList_[i] != -1)
            {
                MPI_Request req;
                reqs.push_back(req);

                MPI_Isend( splitBuffer_[i]->getOutSerialBuffer(),
                          splitBuffer_[i]->getOutSerialBufferSize(),
                          MPI_BYTE, destList_[i], MPI_DATA_TAG, communicator_, &reqs.back());
            }
        }
    }

    if(role == DECAF_REDIST_DEST)
    {
	struct timeval begin;
        struct timeval end;

        gettimeofday(&begin, NULL);
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
                //data->merge();
                data->unserializeAndStore(data->getInSerialBuffer(), nitems);
            }

        }

        // Checking if we have something in transit
        if(transit)
        {
            //data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
            data->unserializeAndStore(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());

            //We don't need it anymore. Cleaning for the next iteration
            transit.reset();
        }

	gettimeofday(&end, NULL);
        timeGlobalRecep += end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

        data->mergeStoredData();
    }
}
*/


void
decaf::
RedistBlockMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
{
    struct timeval beginRedist;
    struct timeval endRedist;
    struct timeval beginMerge;
    struct timeval endMerge;

    gettimeofday(&beginRedist,NULL);
    //Processing the data exchange
    if(role == DECAF_REDIST_SOURCE)
    {
        for(unsigned int i = 0; i <  destList_.size(); i++)
        {
            //Sending to self, we simply copy the string from the out to in
            if(destList_[i] == rank_)
            {
                transit = splitBuffer_[i];
            }
            else if(destList_[i] != -1)
            {
                MPI_Request req;
                reqs.push_back(req);
                MPI_Isend( splitBuffer_[i]->getOutSerialBuffer(),
                           splitBuffer_[i]->getOutSerialBufferSize(),
                           MPI_BYTE, destList_[i], MPI_DATA_TAG, communicator_, &reqs.back());
            }
            else
            {
                MPI_Request req;
                reqs.push_back(req);

                MPI_Isend( NULL,
                           0,
                           MPI_BYTE, i + local_dest_rank_, MPI_DATA_TAG, communicator_, &reqs.back());

            }
        }
        gettimeofday(&endRedist, NULL);
        timeGlobalRedist += endRedist.tv_sec+(endRedist.tv_usec/1000000.0) - beginRedist.tv_sec - (beginRedist.tv_usec/1000000.0);
        
    }

    if(role == DECAF_REDIST_DEST)
    {
        int nbRecep;
        if(isSource()) //Overlapping case
            nbRecep = nbSources_-1;
        else
            nbRecep = nbSources_;

        struct timeval beginReceiv;
        struct timeval endReceiv;
        for(int i = 0; i < nbRecep; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_DATA_TAG, communicator_, &status);
            if (status.MPI_TAG == MPI_DATA_TAG)  // normal, non-null get
            {
                int nitems; // number of items (of type dtype) in the message
                MPI_Get_count(&status, MPI_BYTE, &nitems);

                if(nitems > 0)
                {
                    //Allocating the space necessary
                    data->allocate_serial_buffer(nitems);
                    MPI_Recv(data->getInSerialBuffer(), nitems, MPI_BYTE, status.MPI_SOURCE,
                             status.MPI_TAG, communicator_, &status);

                    // The dynamic type of merge is given by the user
                    // NOTE : examin if it's not more efficient to receive everything
                    // and then merge. Memory footprint more important but allows to
                    // aggregate the allocations etc
                    //data->merge();
                    //data->unserializeAndStore();
                    gettimeofday(&beginReceiv, NULL);
                    data->unserializeAndStore(data->getInSerialBuffer(), nitems);
                    gettimeofday(&endReceiv, NULL);
                    timeGlobalReceiv += endReceiv.tv_sec+(endReceiv.tv_usec/1000000.0) - beginReceiv.tv_sec - (beginReceiv.tv_usec/1000000.0);

                }
                else
                {
                    MPI_Request req;
                    reqs.push_back(req);

                    MPI_Isend( NULL,
                               0,
                               MPI_BYTE, i + local_dest_rank_, MPI_DATA_TAG, communicator_, &reqs.back());

                }
            }
        }
        // Checking if we have something in transit
        if(transit)
        {
            gettimeofday(&beginReceiv, NULL);
            //data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
            data->unserializeAndStore(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
            gettimeofday(&endReceiv, NULL);
            timeGlobalReceiv += endReceiv.tv_sec+(endReceiv.tv_usec/1000000.0) - beginReceiv.tv_sec - (beginReceiv.tv_usec/1000000.0);


            //We don't need it anymore. Cleaning for the next iteration
            transit.reset();
        }
        gettimeofday(&endRedist, NULL);
        timeGlobalRedist += endRedist.tv_sec+(endRedist.tv_usec/1000000.0) - beginRedist.tv_sec - (beginRedist.tv_usec/1000000.0);
        gettimeofday(&beginMerge, NULL);
        data->mergeStoredData();
        gettimeofday(&endMerge, NULL);
        timeGlobalMerge += endMerge.tv_sec+(endMerge.tv_usec/1000000.0) - beginMerge.tv_sec - (beginMerge.tv_usec/1000000.0);
    }

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
    //splitBuffer_.clear(); //We keep the chunks and soft clean them at each iteration
    destList_.clear();
}



