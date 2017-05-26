#ifndef ARRAY_N_DIMENSION_CONSTRUCT_DATA
#define ARRAY_N_DIMENSION_CONSTRUCT_DATA

#include <bredala/data_model/baseconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <boost/serialization/array.hpp>
#include <bredala/data_model/multiarrayserialize.hpp>



// TODO :  See http://www.boost.org/doc/libs/1_46_1/libs/serialization/doc/special.html
// for serialization optimizations


namespace decaf {

template<typename T, int Dim>
class ArrayNDimConstructData : public BaseConstructData {
public:
    ArrayNDimConstructData(mapConstruct map = mapConstruct())
        : BaseConstructData(map){}

    ArrayNDimConstructData(boost::multi_array<T, Dim> value,
                           mapConstruct map = mapConstruct()) :
        BaseConstructData(map), value_(value){}

    virtual ~ArrayNDimConstructData(){}

    virtual bool isBlockSplitable(){ return true; }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
    }

    virtual boost::multi_array<T, Dim> getArray(){ return value_; }

    virtual int getNbItems(){ return value_.size(); }

	virtual std::string getTypename(){ return std::string("ArrayNDim_" + Dim + "_" + boost::typeindex::type_id<T>().pretty_name()); }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        std::cout<<"ERROR : the spliting of a n-dimension array is not implemted yet."<<std::endl;
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
            const std::vector< block3D >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;

        std::cout<<"ERROR : using a dimension non defined"<<std::endl;


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
        std::shared_ptr<ArrayNDimConstructData<T,Dim> >other_ = std::dynamic_pointer_cast<ArrayNDimConstructData<T,Dim> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }




protected:
    boost::multi_array<T, Dim> value_;
};

template<typename T>
std::vector<std::shared_ptr<BaseConstructData> > ArrayNDimConstructData<T,2>::split(
        const std::vector< std::vector<int> >& range,
        std::vector< mapConstruct >& partial_map,
        ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
{
    std::vector<std::shared_ptr<BaseConstructData> > result;
    std::cout<<"Specialized version for 2D version"<<std::endl;

    if(value_.num_dimensions() != 3)
    {
        std::cout<<"ERROR : split function is only implemented for dimension 3."<<std::endl;
        return result;
    }

    for(unsigned int i = 0; i < range.size(); i++)
    {
        // Sanity check
        for(unsigned int d = 0 ; d < 3; d++)
        {
            if(range.at(i).extends[d] > value_.shape()[d])
            {
                std::cout<<"ERROR : trying to cut an array out of boundary."
                        <<"("<<range.at(i).extends[d]<<" available, "<<value_.shape()[d]<<" requested on dimension "<<d<<")"<<std::endl;
                return result;
            }

        }
    }

    // Naive method
    for(unsigned int i = 0; i < range.size(); i++)
    {
        boost::multi_array<T, 3> subArray(boost::extents[range.at(i).extends[0]][range.at(i).extends[1]][range.at(i).extends[2]]);
        for(int x = 0; x < range.at(i).extends[0]; x++)
        {
            for(int y = 0; y < range.at(i).extends[1]; y++)
            {
                for(int z = 0; z < range.at(i).extends[2]; z++)
                    subArray[x][y][z] = value_[range.at(i).base[0]+x][range.at(i).base[1]+y][range.at(i).base[2]+z];
            }
        }

        std::shared_ptr<ArrayNDimConstructData<float,3> > data = std::make_shared<ArrayNDimConstructData<float,3> >(
                    subArray);
        result.push_back(data);
    }

    return result;
}

typedef ArrayNDimConstructData<float,2> ArrayNDimConstructDataf2D;
typedef ArrayNDimConstructData<double,2> ArrayNDimConstructDatad2D;
typedef ArrayNDimConstructData<int,2> ArrayNDimConstructDatai2D;
typedef ArrayNDimConstructData<float,3> ArrayNDimConstructDataf3D;
typedef ArrayNDimConstructData<double,3> ArrayNDimConstructDatad3D;
typedef ArrayNDimConstructData<int,3> ArrayNDimConstructDatai3D;

} // namespace


BOOST_CLASS_EXPORT_GUID(decaf::ArrayNDimConstructDataf2D,"ArrayNDimConstructData<float2>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayNDimConstructDatai2D,"ArrayNDimConstructData<int2>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayNDimConstructDatad2D,"ArrayNDimConstructData<double2>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayNDimConstructDataf3D,"ArrayNDimConstructData<float3>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayNDimConstructDatai3D,"ArrayNDimConstructData<int3>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayNDimConstructDatad3D,"ArrayNDimConstructData<double3>")

#endif
