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

#include <decaf/transport/mpi/redist_zcurve_mpi.h>


namespace decaf
{

unsigned int Morton_3D_Encode_10bit( unsigned int index1, unsigned int index2, unsigned int index3 )
{ // pack 3 10-bit indices into a 30-bit Morton code
  index1 &= 0x000003ff;
  index2 &= 0x000003ff;
  index3 &= 0x000003ff;
  index1 |= ( index1 << 16 );
  index2 |= ( index2 << 16 );
  index3 |= ( index3 << 16 );
  index1 &= 0x030000ff;
  index2 &= 0x030000ff;
  index3 &= 0x030000ff;
  index1 |= ( index1 << 8 );
  index2 |= ( index2 << 8 );
  index3 |= ( index3 << 8 );
  index1 &= 0x0300f00f;
  index2 &= 0x0300f00f;
  index3 &= 0x0300f00f;
  index1 |= ( index1 << 4 );
  index2 |= ( index2 << 4 );
  index3 |= ( index3 << 4 );
  index1 &= 0x030c30c3;
  index2 &= 0x030c30c3;
  index3 &= 0x030c30c3;
  index1 |= ( index1 << 2 );
  index2 |= ( index2 << 2 );
  index3 |= ( index3 << 2 );
  index1 &= 0x09249249;
  index2 &= 0x09249249;
  index3 &= 0x09249249;
  return( index1 | ( index2 << 1 ) | ( index3 << 2 ) );
}

void Morton_3D_Decode_10bit( const unsigned int morton,
                                 unsigned int& index1, unsigned int& index2, unsigned int& index3 )
    { // unpack 3 10-bit indices from a 30-bit Morton code
      unsigned int value1 = morton;
      unsigned int value2 = ( value1 >> 1 );
      unsigned int value3 = ( value1 >> 2 );
      value1 &= 0x09249249;
      value2 &= 0x09249249;
      value3 &= 0x09249249;
      value1 |= ( value1 >> 2 );
      value2 |= ( value2 >> 2 );
      value3 |= ( value3 >> 2 );
      value1 &= 0x030c30c3;
      value2 &= 0x030c30c3;
      value3 &= 0x030c30c3;
      value1 |= ( value1 >> 4 );
      value2 |= ( value2 >> 4 );
      value3 |= ( value3 >> 4 );
      value1 &= 0x0300f00f;
      value2 &= 0x0300f00f;
      value3 &= 0x0300f00f;
      value1 |= ( value1 >> 8 );
      value2 |= ( value2 >> 8 );
      value3 |= ( value3 >> 8 );
      value1 &= 0x030000ff;
      value2 &= 0x030000ff;
      value3 &= 0x030000ff;
      value1 |= ( value1 >> 16 );
      value2 |= ( value2 >> 16 );
      value3 |= ( value3 >> 16 );
      value1 &= 0x000003ff;
      value2 &= 0x000003ff;
      value3 &= 0x000003ff;
      index1 = value1;
      index2 = value2;
      index3 = value3;
    }



bool
decaf::
RedistZCurveMPI::isSource()
{
    return rank_ >= local_source_rank_ && rank_ < local_source_rank_ + nbSources_;
}

bool
decaf::
RedistZCurveMPI::isDest()
{
    return rank_ >= local_dest_rank_ && rank_ < local_dest_rank_ + nbDests_;
}

// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistZCurveMPI::RedistZCurveMPI(int rankSource,
                                 int nbSources,
                                 int rankDest,
                                 int nbDests,
                                 CommHandle world_comm,
                                 std::vector<float> bBox,
                                 std::vector<int> slices) :
    RedistComp(rankSource, nbSources, rankDest, nbDests),
    communicator_(MPI_COMM_NULL),
    commSources_(MPI_COMM_NULL),
    commDests_(MPI_COMM_NULL),
    bBBox_(false),
    bBox_(bBox),
    slices_(slices)
{
    MPI_Group group, groupRedist, groupSource, groupDest;
    MPI_Comm_group(world_comm, &group);
    int world_rank, world_size;

    MPI_Comm_rank(world_comm, &world_rank);
    MPI_Comm_size(world_comm, &world_size);

    local_source_rank_ = 0;                     // rank of first source in communicator_
    local_dest_rank_   = nbSources;             // rank of first destination in communicator_

    // group covering both the sources and destinations
    // destination ranks need not be higher than source ranks
    int range_both[2][3];
    range_both[0][0] = rankSource;
    range_both[0][1] = rankSource + nbSources - 1;
    range_both[0][2] = 1;
    range_both[1][0] = rankDest;
    range_both[1][1] = rankDest + nbDests - 1;
    range_both[1][2] = 1;

    MPI_Group_range_incl(group, 2, range_both, &groupRedist);
    MPI_Comm_create_group(world_comm, groupRedist, 0, &communicator_);
    MPI_Comm_rank(communicator_, &rank_);
    MPI_Comm_size(communicator_, &size_);

    // group with all the sources
    if(isSource())
    {
        int range_src[3];
        range_src[0] = rankSource;
        range_src[1] = rankSource + nbSources - 1;
        range_src[2] = 1;
        MPI_Group_range_incl(group, 1, &range_src, &groupSource);
        MPI_Comm_create_group(world_comm, groupSource, 0, &commSources_);
        MPI_Group_free(&groupSource);
        int source_rank;
        MPI_Comm_rank(commSources_, &source_rank);
    }

    // group with all the destinations
    if(isDest())
    {
        int range_dest[3];
        range_dest[0] = rankDest;
        range_dest[1] = rankDest + nbDests - 1;
        range_dest[2] = 1;
        MPI_Group_range_incl(group, 1, &range_dest, &groupDest);
        MPI_Comm_create_group(world_comm, groupDest, 0, &commDests_);
        MPI_Group_free(&groupDest);
        int dest_rank;
        MPI_Comm_rank(commDests_, &dest_rank);
    }

    MPI_Group_free(&group);
    MPI_Group_free(&groupRedist);

    // The slices are the number rows in each dimension for the grid
    if(slices_.size() != 3)
    {
        std::cout<<"WARNING : No slices given, will use 8,8,8"<<std::endl;
        slices_ = { 8,8,8 };
    }
    else
    {
        for(unsigned int i = 0; i < 3; i++)
        {
            if(slices_.at(i) < 1)
            {
                std::cout<<" ERROR : slices can't be inferior to 1. Switching the value to 1."<<std::endl;
                slices_.at(i) = 1;
            }
        }
    }

    //Computing the index ranges per destination
    //We can use -1 because the constructor makes sure that all items of slices_ are > 0
    int maxIndex = Morton_3D_Encode_10bit(slices_[0]-1, slices_[1]-1, slices_[2]-1);

    indexes_per_dest_ = maxIndex /  nbDests_;
    // From rank 0 to rankOffset_ - 1, each destination are assigned to indexes_per_dest_+1 indexes
    // From rankOffset to the rest, each destination are assigned to indexes_per_dest_ indexes
    rankOffset_ = maxIndex %  nbDests_;

    // Checking the bounding box and updating the slicesDelta if possible
    // If the bounding box is not available, this will be filled during the first
    // computeGlobal() call
    if(bBox_.size() == 6)
    {
        bBBox_ = true;
        slicesDelta_.resize(3);
        slicesDelta_[0] = (bBox_[3] - bBox_[0]) / (float)(slices_[0]);
        slicesDelta_[1] = (bBox_[4] - bBox_[1]) / (float)(slices_[1]);
        slicesDelta_[2] = (bBox_[5] - bBox_[2]) / (float)(slices_[2]);
    }

    destBuffer_ = new int[nbDests_];
    sum_ = new int[nbDests_];
}

decaf::
RedistZCurveMPI::~RedistZCurveMPI()
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

void
decaf::
RedistZCurveMPI::computeGlobal(std::shared_ptr<BaseData> data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // If we don't have the global bounding box, we compute it once
        if(!bBBox_)
        {

            if(!data->hasZCurveKey())
            {
                std::cout<<"ERROR : Trying to redistribute the data with respect to a ZCurve "
                        <<"but no ZCurveKey (position) is available in the data. Abording."<<std::endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }

            int nbParticules; // The size of the array is 3*nbParticules
            const float* key = data->getZCurveKey(&nbParticules);
            if(key == NULL){
                std::cout<<"ERROR : the key pointer is NULL"<<std::endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }

            float localBBox[6];

            // Computing the local bounding box
            if(nbParticules > 0)
            {
                for(unsigned int i = 0; i < 6; i++)
                    localBBox[i] = key[i % 3];

                for(unsigned int i = 1; i < nbParticules; i++)
                {
                    for(unsigned int j = 0; j < 3; j++)
                    {
                        if(key[i*3+j] < localBBox[j])
                            localBBox[j] = key[i*3+j];
                        if(key[i*3+j] > localBBox[3+j])
                            localBBox[3+j] = key[i*3+j];
                    }
                }

                //Aggregating the bounding boxes to get the global one
                bBox_.resize(6);
                MPI_Allreduce(&(localBBox[0]), &(bBox_[0]), 3, MPI_FLOAT, MPI_MIN, commSources_);
                MPI_Allreduce(&(localBBox[0])+3, &(bBox_[0])+3, 3, MPI_FLOAT, MPI_MAX, commSources_);

                bBBox_ = true;

                //Updating the slices
                slicesDelta_.resize(3);
                slicesDelta_[0] = (bBox_[3] - bBox_[0]) / (float)(slices_[0]);
                slicesDelta_[1] = (bBox_[4] - bBox_[1]) / (float)(slices_[1]);
                slicesDelta_[2] = (bBox_[5] - bBox_[2]) / (float)(slices_[2]);
            }
            else
            {
                std::cout<<"ERROR : no particules present on one process. Case not handled. Abording"<<std::endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        }
    }
}

void
decaf::
RedistZCurveMPI::splitData(std::shared_ptr<BaseData> data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE){

        //Compute the split vector and the destination ranks
        std::vector<std::vector<int> > split_ranges = std::vector<std::vector<int> >( nbDests_);
        int nbParticules;
        const float* pos = data->getZCurveKey(&nbParticules);

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete  summerizeDest_;
         summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything


        for(int i = 0; i < nbParticules; i++)
        {
            //Computing the cell of the particule
            int x = (pos[3*i] - bBox_[0]) / slicesDelta_[0];
            int y = (pos[3*i+1] - bBox_[1]) / slicesDelta_[1];
            int z = (pos[3*i+2] - bBox_[2]) / slicesDelta_[2];

            //Safety in case of wrong rounding
            if(x < 0) x = 0;
            if(y < 0) y = 0;
            if(z < 0) z = 0;
            if(x >= slices_[0]) x = slices_[0] - 1;
            if(y >= slices_[1]) y = slices_[1] - 1;
            if(z >= slices_[2]) z = slices_[2] - 1;

            unsigned int morton = Morton_3D_Encode_10bit(x,y,z);

            //Computing the destination rank
            int destination;
            if(morton < rankOffset_ * (indexes_per_dest_+1))
            {
                destination = morton / (indexes_per_dest_+1);
            }
            else
            {
                morton -= rankOffset_ * (indexes_per_dest_+1);
                destination = rankOffset_ + morton / indexes_per_dest_;
            }

            //Use for the case where morton == maxMorton
            if(destination == nbDests_)
                destination = nbDests_ - 1;

            split_ranges[destination].push_back(i);

        }

        //Updating the informations about messages to send
        for(unsigned int i = 0; i < split_ranges.size(); i++)
        {
            if(split_ranges.at(i).size() > 0)
            {
                destList_.push_back(i + local_dest_rank_);

                //We won't send a message if we send to self
                if(i + local_dest_rank_ != rank_)
                    summerizeDest_[i] = 1;
            }
            else
                destList_.push_back(-1);
        }


        splitChunks_ =  data->split( split_ranges );

        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            // TODO : Check the rank for the destination.
            // Not necessary to serialize if overlapping
            if(!splitChunks_.at(i)->serialize())
                std::cout<<"ERROR : unable to serialize one object"<<std::endl;
        }

        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping
        data->purgeData();
    }
}

void
decaf::
RedistZCurveMPI::redistribute(std::shared_ptr<BaseData> data, RedistRole role)
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
RedistZCurveMPI::merge(RedistRole role)
{
    return std::shared_ptr<BaseData>();
}

void
decaf::
RedistZCurveMPI::flush()
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

