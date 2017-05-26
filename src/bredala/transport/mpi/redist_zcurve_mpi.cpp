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

#include <bredala/transport/mpi/redist_zcurve_mpi.h>


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

} // namespace

decaf::
RedistZCurveMPI::RedistZCurveMPI(int rankSource,
                                 int nbSources,
                                 int rankDest,
                                 int nbDests,
                                 CommHandle world_comm,
                                 RedistCommMethod commMethod,
                                 MergeMethod mergeMethod,
                                 std::vector<float> bBox,
                                 std::vector<int> slices) :
    bBBox_(false),
    bBox_(bBox),
    slices_(slices),
    RedistMPI(rankSource, nbSources, rankDest, nbDests, world_comm, commMethod, mergeMethod)
{

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
                std::cout<<" ERROR : slices can't be inferior to 1. Switching the value to 1."
                         <<std::endl;
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
}

void
decaf::
RedistZCurveMPI::computeGlobal(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // If we don't have the global bounding box, we compute it once
        if(!bBBox_)
        {
            if(!data->isCountable())
            {
                std::cout<<"ERROR : Trying to redistribute the data with respect to a ZCurve "
                         <<"but the data is not fully countable. Abording."<<std::endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }

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
RedistZCurveMPI::splitData(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE){

        //Compute the split vector and the destination ranks
        std::vector<std::vector<int> > split_ranges = std::vector<std::vector<int> >( nbDests_);
        int nbParticules;
        const float* pos = data->getZCurveKey(&nbParticules);

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        if( summerizeDest_) delete [] summerizeDest_;
        summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        std::vector<unsigned int> sumPos(nbDests_, 0); //To count the amount of particles per
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

            // Updating the ranges
            if(split_ranges[destination].empty())
            {
                split_ranges[destination].push_back(i);
                split_ranges[destination].push_back(1);
            }
            // Case where the current position is following the previous one
            else if(split_ranges[destination].back() + (split_ranges[destination])[split_ranges[destination].size()-2] == i )
            {
                split_ranges[destination].back() = split_ranges[destination].back() + 1;
            }
            else
            {
                split_ranges[destination].push_back(i);
                split_ranges[destination].push_back(1);
            }

            sumPos[destination] = sumPos[destination] + 1;

        }

        // Pushing the totals in the ranges
        for(unsigned int i = 0; i < split_ranges.size(); i++)
            split_ranges[i].push_back(sumPos[i]);

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

        if(splitBuffer_.empty())
            // We prealloc with 0 to avoid allocating too much iterations
            // The first iteration will make a reasonable allocation
            data.preallocMultiple(nbDests_, 0, splitBuffer_);
        else
        {
            // No need to adjust the number of buffer, always equal to the number of destination
            for(unsigned int i = 0; i < splitBuffer_.size(); i++)
                splitBuffer_[i]->softClean();
        }


        data->split( split_ranges, splitBuffer_ );

        for(unsigned int i = 0; i < splitBuffer_.size(); i++)
            splitChunks_.push_back(splitBuffer_[i]);

        for(unsigned int i = 0; i < splitChunks_.size(); i++)
        {
            // TODO : Check the rank for the destination.
            // Not necessary to serialize if overlapping
            if(!splitChunks_[i]->serialize())
                std::cout<<"ERROR : unable to serialize one object"<<std::endl;
        }

        // DEPRECATED
        // Everything is done, now we can clean the data.
        // Data might be rewriten if producers and consummers are overlapping
        // data->purgeData();
    }
}
