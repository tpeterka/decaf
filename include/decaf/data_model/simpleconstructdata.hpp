#ifndef SIMPLE_CONSTRUCT_DATA
#define SIMPLE_CONSTRUCT_DATA

#include "baseconstructdata.hpp"

namespace decaf{


template<typename T>
class SimpleConstructData : public BaseConstructData {
public:
    SimpleConstructData() : BaseConstructData(){}
    SimpleConstructData(T value) : value_(value), BaseConstructData(){}
    virtual ~SimpleConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
    }

    virtual int getNbItems(){ return 1; }

    T& getData(){ return value_; }

    virtual std::vector< std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_));
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_));
                break;
            }
            case DECAF_SPLIT_MINUS_NBITEM:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(range.at(i)));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for SimpleConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_));
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_));
                break;
            }
            case DECAF_SPLIT_MINUS_NBITEM:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(range.at(i).size()));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for SimpleConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual bool merge(std::shared_ptr<BaseConstructData> other,
              ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<SimpleConstructData<T> > other_ = dynamic_pointer_cast<SimpleConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                if(value_ != other_->value_)
                {
                    std::cout<<"ERROR : the original and other data do not have the same data."
                    <<"Default policy keep one value and check that the 2 marge values are"
                    <<" the same. Make sure the values are the same or change the merge policy"<<std::endl;
                    return false;
                }
                return true;
                break;
            }
            case DECAF_MERGE_FIRST_VALUE: //We don't have to do anything here
            {
                return true;
                break;
            }
            case DECAF_MERGE_ADD_VALUE:
            {
                value_ = value_ + other_->value_;
                return true;
                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for simple data."<<std::endl;
                return false;
                break;
            }

        }
    }

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other)
    {
        std::shared_ptr<SimpleConstructData<T> > other_ = dynamic_pointer_cast<SimpleConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }

protected:
    T value_;
};

}//namespace

BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<float>,"SimpleConstructData<float>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<int>,"SimpleConstructData<int>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<double>,"SimpleConstructData<double>")
BOOST_CLASS_EXPORT_GUID(decaf::SimpleConstructData<char>,"SimpleConstructData<char>")


#endif
