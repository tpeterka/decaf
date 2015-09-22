#ifndef ARRAY_3D_CONSTRUCT_DATA
#define ARRAY_3D_CONSTRUCT_DATA

#include <decaf/data_model/baseconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <boost/serialization/array.hpp>
#include <decaf/data_model/multiarrayserialize.hpp>
#include <decaf/data_model/block.hpp>



// TODO :  See http://www.boost.org/doc/libs/1_46_1/libs/serialization/doc/special.html
// for serialization optimizations


namespace decaf {

template<typename T>
class Array3DConstructData : public BaseConstructData {
public:
    Array3DConstructData(mapConstruct map = mapConstruct())
        : BaseConstructData(map){}

    Array3DConstructData(boost::multi_array<T, 3> value,
                           mapConstruct map = mapConstruct()) :
        BaseConstructData(map), value_(value){}

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
        std::cout<<"Specialized version for 3D version"<<std::endl;


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

                if(range.at(i).localExtends_[d+3] > value_.shape()[d])
                {
                    std::cout<<"ERROR : trying to cut an array out of boundary."
                            <<"("<<range.at(i).localExtends_[d+3]<<" available, "<<value_.shape()[d]<<" requested on dimension "<<d<<")"<<std::endl;
                    return result;
                }

            }
        }

        // Naive method
        for(unsigned int i = 0; i < range.size(); i++)
        {
            boost::multi_array<T, 3> subArray(boost::extents[range.at(i).localExtends_[0]][range.at(i).localExtends_[1]][range.at(i).localExtends_[2]]);
            for(int x = 0; x < range.at(i).localExtends_[3]; x++)
            {
                for(int y = 0; y < range.at(i).localExtends_[4]; y++)
                {
                    for(int z = 0; z < range.at(i).localExtends_[5]; z++)
                        subArray[x][y][z] = value_[range.at(i).localExtends_[0]+x][range.at(i).localExtends_[1]+y][range.at(i).localExtends_[2]+z];
                }
            }

            std::shared_ptr<Array3DConstructData<T> > data = std::make_shared<Array3DConstructData<T> >(
                        subArray);

            //TODO : should maintain a Block structure
            result.push_back(data);
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
