#ifndef ARRAY_3D_CONSTRUCT_DATA
#define ARRAY_3D_CONSTRUCT_DATA

#include <decaf/data_model/baseconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <boost/serialization/array.hpp>
#include <decaf/data_model/multiarrayserialize.hpp>
#include <decaf/data_model/block.hpp>
#include <decaf/data_model/blockconstructdata.hpp>
#include <math.h>



// TODO :  See http://www.boost.org/doc/libs/1_46_1/libs/serialization/doc/special.html
// for serialization optimizations

namespace decaf {


//Tool function to extract the overlap between to intervale
bool hasOverlap(unsigned int baseMin, unsigned int baseMax,
                unsigned int otherMin, unsigned int otherMax,
                unsigned int & overlapMin, unsigned int & overlapMax)
{
    if(otherMax < baseMin || otherMin > baseMax )
        return false;

    overlapMin = std::max(baseMin, otherMin);
    overlapMax = std::min(baseMax, otherMax);
    return true;
}

template<typename T>
void copy3DArray(boost::multi_array<T, 3>* dest, Block<3>& blockDest,
                 boost::multi_array<T, 3>* src, Block<3>& blockSrc)
{
    assert(blockDest.hasLocalExtends_ && blockSrc.hasLocalExtends_);

    int offsetX = blockSrc.localExtends_[0] - blockDest.localExtends_[0];
    int offsetY = blockSrc.localExtends_[1] - blockDest.localExtends_[1];
    int offsetZ = blockSrc.localExtends_[2] - blockDest.localExtends_[2];

    assert(offsetX >= 0 && offsetY >= 0 && offsetZ >= 0);

    for(unsigned int x = 0; x < src->shape()[0]; x++)
    {
        for(unsigned int y = 0; y < src->shape()[1]; y++)
        {
            for(unsigned int z = 0; z < src->shape()[2]; z++)
                (*dest)[x + offsetX][y + offsetY][z + offsetZ] += (*src)[x][y][z];
        }
    }
}

template<typename T>
class Array3DConstructData : public BaseConstructData {
public:
    Array3DConstructData(mapConstruct map = mapConstruct())
        : BaseConstructData(map), value_(NULL), isOwner_(false){}

    Array3DConstructData(boost::multi_array<T, 3> *value, Block<3> block = Block<3>(),
                           bool isOwner = false, mapConstruct map = mapConstruct()) :
        BaseConstructData(map), value_(value), block_(block), isOwner_(isOwner){}

    virtual ~Array3DConstructData()
    {
        if(isOwner_)
            delete value_;
    }

    virtual bool isBlockSplitable(){ return true; }

    void setBlock(Block<3> block){ block_ = block; }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
        ar & BOOST_SERIALIZATION_NVP(block_);
    }

    virtual boost::multi_array<T, 3>* getArray(){ return value_; }

    virtual int getNbItems(){ return value_->shape()[0] * value_->shape()[1] * value_->shape()[2]; }

    virtual bool appendItem(std::shared_ptr<BaseConstructData> dest, unsigned int index, ConstructTypeMergePolicy = DECAF_MERGE_DEFAULT)
    {
	std::cout<<"ERROR : appendItem not implemented yet for 3d arrays."<<std::endl;
	return false;
    }

    virtual void preallocMultiple(int nbCopies , int nbItems, std::vector<std::shared_ptr<BaseConstructData> >& result)
    {
	std::cout<<"ERROR : prealloc multiple not available for 3d arrays."<<std::endl;
	return;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        std::cout<<"ERROR : the spliting of a 3-dimension array is not implemted yet."<<std::endl;
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        std::cout<<"ERROR : the spliting of a n-dimension array is not implemted yet."<<std::endl;
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< Block<3> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {

        std::vector<std::shared_ptr<BaseConstructData> > result;

        for(unsigned int i = 0; i < range.size(); i++)
        {

            if(!range.at(i).hasLocalExtends_)
            {
                std::cerr<<"ERROR : the block "<<i<<" doesn't have a local box to split a 3d array."<<std::endl;
                return result;
            }

            if(!block_.hasLocalExtends_)
            {
                std::cerr<<"ERROR : the block of the 3d array doesn't have informations about the local grid"<<std::endl;
                return result;
            }
        }

        // Naive method to write the subarray
        unsigned int startX, endX, startY, endY, startZ, endZ;
        startX = block_.localExtends_[0];
        endX = block_.localExtends_[0] + block_.localExtends_[3];
        startY = block_.localExtends_[1];
        endY = block_.localExtends_[1] + block_.localExtends_[4];
        startZ = block_.localExtends_[2];
        endZ = block_.localExtends_[2] + block_.localExtends_[5];


        for(unsigned int i = 0; i < range.size(); i++)
        {
            //Computing the window of data to cut for each dimension
            unsigned int startSplitX, endSplitX, startSplitY, endSplitY, startSplitZ, endSplitZ;
            startSplitX = range.at(i).localExtends_[0];
            endSplitX = range.at(i).localExtends_[0] + range.at(i).localExtends_[3];
            startSplitY = range.at(i).localExtends_[1];
            endSplitY = range.at(i).localExtends_[1] + range.at(i).localExtends_[4];
            startSplitZ = range.at(i).localExtends_[2];
            endSplitZ = range.at(i).localExtends_[2] + range.at(i).localExtends_[5];

            unsigned int xmin, xmax, ymin, ymax, zmin, zmax;
            bool overlapX = hasOverlap(startX, endX, startSplitX, endSplitX, xmin, xmax);
            bool overlapY = hasOverlap(startY, endY, startSplitY, endSplitY, ymin, ymax);
            bool overlapZ = hasOverlap(startZ, endZ, startSplitZ, endSplitZ, zmin, zmax);
            unsigned int dx = xmax - xmin, dy = ymax - ymin, dz = zmax - zmin;

            if(overlapX && overlapY && overlapZ)
            {
                //There is an intersection between the blocks
                boost::multi_array<T, 3>* subArray = new boost::multi_array<T, 3>(boost::extents[dx][dy][dz]);
                for(int x = xmin; x < xmax; x++)
                {
                    for(int y = ymin; y < ymax; y++)
                    {
                        for(int z = zmin; z < zmax; z++)
                        {
                            (*subArray)[x-xmin][y-ymin][z-zmin] =
                                    (*value_)[x-startX][y-startY][z-startZ];
                        }
                    }
                }

                Block<3> subBlock = block_;

                subBlock.setLocalExtends({xmin, ymin, zmin, xmax - xmin, ymax - ymin, zmax - zmin});
                subBlock.setLocalBBox({ subBlock.globalBBox_[0] + xmin * subBlock.gridspace_,
                                        subBlock.globalBBox_[1] + ymin * subBlock.gridspace_,
                                        subBlock.globalBBox_[2] + zmin * subBlock.gridspace_,
                                        (xmax - xmin) * subBlock.gridspace_,
                                        (ymax - ymin) * subBlock.gridspace_,
                                        (zmax - zmin) * subBlock.gridspace_});

                // Updating the own information as well
                if(range.at(i).hasOwnExtends_)
                {
                    startSplitX = range.at(i).ownExtends_[0];
                    endSplitX = range.at(i).ownExtends_[0] + range.at(i).ownExtends_[3];
                    startSplitY = range.at(i).ownExtends_[1];
                    endSplitY = range.at(i).ownExtends_[1] + range.at(i).ownExtends_[4];
                    startSplitZ = range.at(i).ownExtends_[2];
                    endSplitZ = range.at(i).ownExtends_[2] + range.at(i).ownExtends_[5];

                    overlapX = hasOverlap(startX, endX, startSplitX, endSplitX, xmin, xmax);
                    overlapY = hasOverlap(startY, endY, startSplitY, endSplitY, ymin, ymax);
                    overlapZ = hasOverlap(startZ, endZ, startSplitZ, endSplitZ, zmin, zmax);

                    if(overlapX && overlapY && overlapZ)
                    {
                        subBlock.ownExtends_ = {xmin, ymin, zmin, xmax - xmin, ymax - ymin, zmax - zmin};
                        subBlock.ownBBox_ = { subBlock.globalBBox_[0] + xmin * subBlock.gridspace_,
                                                subBlock.globalBBox_[1] + ymin * subBlock.gridspace_,
                                                subBlock.globalBBox_[2] + zmin * subBlock.gridspace_,
                                                (xmax - xmin) * subBlock.gridspace_,
                                                (ymax - ymin) * subBlock.gridspace_,
                                                (zmax - zmin) * subBlock.gridspace_};

                        if(subBlock.localExtends_ == subBlock.ownExtends_)
                            subBlock.ghostSize_ = 0;
                        else
                        {
                            subBlock.ghostSize_ = std::max(subBlock.ownExtends_[0] - subBlock.localExtends_[0],
                                     subBlock.ownExtends_[1] - subBlock.localExtends_[1]);
                            subBlock.ghostSize_ = std::max(subBlock.ghostSize_,
                                     subBlock.ownExtends_[2] - subBlock.localExtends_[2]);
                            subBlock.ghostSize_ = std::max(subBlock.ghostSize_,
                                     subBlock.localExtends_[0] - subBlock.ownExtends_[0]);
                            subBlock.ghostSize_ = std::max(subBlock.ghostSize_,
                                     subBlock.localExtends_[1] - subBlock.ownExtends_[1]);
                            subBlock.ghostSize_ = std::max(subBlock.ghostSize_,
                                     subBlock.localExtends_[2] - subBlock.ownExtends_[2]);
                        }

                    }
                }
                else
                {
                    subBlock.ghostSize_ = 0;
                    subBlock.setOwnExtends(subBlock.localExtends_);
                    subBlock.setOwnBBox(subBlock.localBBox_);

                }
                std::shared_ptr<Array3DConstructData<T> > data = std::make_shared<Array3DConstructData<T> >(
                            subArray, subBlock, true);

                result.push_back(data);

            }
            else
            {
                //No data here
                boost::multi_array<T, 3> *subArray = new boost::multi_array<T, 3>(boost::extents[0][0][0]);
                Block<3> subBlock = block_;

                subBlock.localExtends_ = {0, 0, 0, 0, 0, 0};
                subBlock.localBBox_ = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
                subBlock.ownExtends_ = subBlock.localExtends_;
                subBlock.ownBBox_ = subBlock.localBBox_;
                subBlock.ghostSize_ = 0;

                std::shared_ptr<Array3DConstructData<T> > data = std::make_shared<Array3DConstructData<T> >(
                            subArray, subBlock, true);


                result.push_back(data);
            }

        }


        return result;
    }

    virtual void split(
            const std::vector< std::vector<int> >& range,
	    std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
	return;
    }

    virtual bool merge( std::shared_ptr<BaseConstructData> other,
                        mapConstruct partial_map,
                        ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                //Getting the shape of the merged array
                std::map<std::string, datafield>::iterator field = partial_map->find("domain_block");
                if(field == partial_map->end())
                {
                    std::cerr<<"ERROR : unable to find the field \'domain_block\' "
                             <<"required to merge array3dconstructdata type."<<std::endl;
                    return false;
                }
                std::shared_ptr<BlockConstructData> domainBlock =
                        std::dynamic_pointer_cast<BlockConstructData>(std::get<3>(field->second));


                Block<3>& domain = domainBlock->getData();

                std::shared_ptr<Array3DConstructData<T> >other_ =
                        std::dynamic_pointer_cast<Array3DConstructData<T> >(other);

                // Checking if the current array has the correct shape
                if(block_.hasSameExtends(domain))
                {
                    // The block has already the correct shape, we simply add the
                    // values from the other array
                    copy3DArray(value_, block_, other_->value_, other_->block_);
                }
                else
                {
                    //Creating the new array with the correct shape
                    boost::multi_array<T, 3> *newArray = new boost::multi_array<T, 3>(boost::extents[domain.localExtends_[3]][domain.localExtends_[4]][domain.localExtends_[5]]);

                    //Adding the current array
                    copy3DArray(newArray, domain, value_, block_);

                    //Adding the other array
                    copy3DArray(newArray, domain, other_->value_, other_->block_);

                    //We can now replace the old array
                    if(isOwner_) delete value_;
                    value_ = newArray; //TODO : check the memory of this, probably copy
                    block_ = domain;
                    isOwner_ = true;
                }

                break;
            }
            default:
            {
                std::cerr<<"ERROR : Policy "<<policy<<" not supported for Array3constructData."<<std::endl;
                return false;
            }

        }

        return true;
    }

    virtual bool merge(std::vector<std::shared_ptr<BaseConstructData> >& others,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
	std::cout<<"ERROR : merge multiples partial data is not implemented in array3d."<<std::endl;
	return false;
    }

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other)
    {
        std::shared_ptr<Array3DConstructData<T> >other_ = std::dynamic_pointer_cast<Array3DConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }

    virtual void softClean()
    {
	std::cout<<"ERROR : soft clean not implemented yet for 3d array."<<std::endl;
	return;	
    }


protected:
    boost::multi_array<T, 3>* value_;
    Block<3> block_;
    bool isOwner_;
};


} // namespace

BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<float>,"ArrayNDimConstructData<float3>")
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<int>,"ArrayNDimConstructData<int3>")
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<double>,"ArrayNDimConstructData<double3>")

#endif
