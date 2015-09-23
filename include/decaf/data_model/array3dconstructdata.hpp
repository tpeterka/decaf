#ifndef ARRAY_3D_CONSTRUCT_DATA
#define ARRAY_3D_CONSTRUCT_DATA

#include <decaf/data_model/baseconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <boost/serialization/array.hpp>
#include <decaf/data_model/multiarrayserialize.hpp>
#include <decaf/data_model/block.hpp>
#include <math.h>



// TODO :  See http://www.boost.org/doc/libs/1_46_1/libs/serialization/doc/special.html
// for serialization optimizations


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


namespace decaf {

template<typename T>
class Array3DConstructData : public BaseConstructData {
public:
    Array3DConstructData(mapConstruct map = mapConstruct())
        : BaseConstructData(map){}

    Array3DConstructData(boost::multi_array<T, 3> value, Block<3> block = Block<3>(),
                           mapConstruct map = mapConstruct()) :
        BaseConstructData(map), value_(value), block_(block){}

    virtual ~Array3DConstructData(){}

    virtual bool isBlockSplitable(){ return true; }

    void setBlock(Block<3> block){ block_ = block; }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
        ar & BOOST_SERIALIZATION_NVP(block_);
    }

    virtual boost::multi_array<T, 3> getArray(){ return value_; }

    virtual int getNbItems(){ return value_.size(); }

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
            // Sanity check
            for(unsigned int d = 0 ; d < 3; d++)
            {
                if(!range.at(i).hasLocalExtends_)
                {
                    std::cerr<<"ERROR : one block doesn't have a local box to split a 3d array."<<std::endl;
                    return result;
                }
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
                boost::multi_array<T, 3> subArray(boost::extents[dx][dy][dz]);
                for(int x = xmin; x < xmax; x++)
                {
                    for(int y = ymin; y < ymax; y++)
                    {
                        for(int z = zmin; z < zmax; z++)
                        {
                            subArray[x-xmin][y-ymin][z-zmin] =
                                    value_[x-startX][y-startY][z-startZ];
                        }
                    }
                }

                Block<3> subBlock = block_;

                subBlock.localExtends_ = {xmin, ymin, zmin, xmax - xmin, ymax - ymin, zmax - zmin};
                subBlock.localBBox_ = { subBlock.globalBBox_[0] + xmin * subBlock.gridspace_,
                                        subBlock.globalBBox_[1] + ymin * subBlock.gridspace_,
                                        subBlock.globalBBox_[2] + zmin * subBlock.gridspace_,
                                        (xmax - xmin) * subBlock.gridspace_,
                                        (ymax - ymin) * subBlock.gridspace_,
                                        (zmax - zmin) * subBlock.gridspace_};

                // TODO : manage the own as well

                std::shared_ptr<Array3DConstructData<T> > data = std::make_shared<Array3DConstructData<T> >(
                            subArray, subBlock);

                result.push_back(data);

            }
            else
            {
                //No data here
                boost::multi_array<T, 3> subArray(boost::extents[0][0][0]);
                Block<3> subBlock = block_;

                subBlock.localExtends_ = {0, 0, 0, 0, 0, 0};
                subBlock.localBBox_ = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

                std::shared_ptr<Array3DConstructData<T> > data = std::make_shared<Array3DConstructData<T> >(
                            subArray, subBlock);


                result.push_back(data);
            }

        }


        return result;
    }

    virtual bool merge( std::shared_ptr<BaseConstructData> other,
                        mapConstruct partial_map,
                        ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
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




protected:
    boost::multi_array<T, 3> value_;
    Block<3> block_;
};


} // namespace

BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<float>,"ArrayNDimConstructData<float3>")
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<int>,"ArrayNDimConstructData<int3>")
BOOST_CLASS_EXPORT_GUID(decaf::Array3DConstructData<double>,"ArrayNDimConstructData<double3>")

#endif
