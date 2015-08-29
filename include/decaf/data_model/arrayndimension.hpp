#ifndef ARRAY_N_DIMENSION_CONSTRUCT_DATA
#define ARRAY_N_DIMENSION_CONSTRUCT_DATA

#include <decaf/data_model/baseconstructdata.hpp>
#include <boost/multi_array.hpp>
#include <boost/serialization/array.hpp>
#include <decaf/data_model/multiarrayserialize.hpp>



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

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< block3D >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
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
