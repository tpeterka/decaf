#ifndef VECTOR_CONSTRUCT_DATA
#define VECTOR_CONSTRUCT_DATA

#include "baseconstructdata.hpp"

namespace decaf {

template<typename T>
class VectorConstructData : public BaseConstructData {
public:
    VectorConstructData(mapConstruct map = mapConstruct())
        : BaseConstructData(map){}

    VectorConstructData(std::vector<T>& value, int element_per_items, mapConstruct map = mapConstruct()) :
                       BaseConstructData(map), value_(value),
                       element_per_items_(element_per_items){}
    VectorConstructData(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end,
                       int element_per_items, mapConstruct map = mapConstruct()) :
        element_per_items_(element_per_items), BaseConstructData(map)
    {
        value_ = std::vector<T>(begin, end);
    }
    VectorConstructData(T* Vector, int size, int element_per_items, mapConstruct map = mapConstruct()) :
                        element_per_items_(element_per_items), BaseConstructData(map)
    {
        value_.resize(size);
        value_ = std::vector<T>(Vector, Vector + size);
    }

    virtual ~VectorConstructData(){}

    virtual bool isBlockSplitable(){ return false; }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
        ar & BOOST_SERIALIZATION_NVP(element_per_items_);
    }

    virtual std::vector<T>& getVector(){ return value_; }

    virtual int getNbItems(){ return value_.size() / element_per_items_; }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range.at(i);
                if(totalRange != getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return result;
                }

                typename std::vector<T>::iterator it = value_.begin();
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<VectorConstructData<T> > sub =
                            std::make_shared<VectorConstructData<T> >(it, it+(range.at(i)*element_per_items_), element_per_items_);
                    it  = it+(range.at(i)*element_per_items_);
                    result.push_back(sub);
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                std::shared_ptr<VectorConstructData> sub = std::make_shared<VectorConstructData>(
                            value_.begin(), value_.end(), element_per_items_);
                result.push_back(sub);
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for VectorConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range.at(i).size();
                if(totalRange != getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return result;
                }

                typename std::vector<T>::iterator it = value_.begin();
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::vector<T> temp;
                    for(unsigned int j = 0; j< range.at(i).size(); j++)
                        temp.insert( temp.end(),
                                     it+(range.at(i).at(j)*element_per_items_),
                                     it+((range.at(i).at(j)+1)*element_per_items_)
                                     );

                    std::shared_ptr<VectorConstructData<T> > sub =
                            std::make_shared<VectorConstructData<T> >(temp.begin(), temp.end(), element_per_items_);
                    result.push_back(sub);
                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for VectorConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    //This function should never be called
    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< Block<3> >& range,
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
        std::shared_ptr<VectorConstructData<T> > other_ = std::dynamic_pointer_cast<VectorConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                if(value_ != other_->value_)
                {
                    std::cout<<"ERROR : The original and other data do not have the same data."
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
            case DECAF_MERGE_APPEND_VALUES:
            {
                value_.insert(value_.end(), other_->value_.begin(), other_->value_.end());
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
        std::shared_ptr<VectorConstructData<T> >other_ = std::dynamic_pointer_cast<VectorConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }



protected:

    std::vector<T> value_;
    int element_per_items_; //One semantic item is composed of element_per_items_ items in the vector
};

} //namespace

// DEPRECATED; moved to boost_macros.h
// BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<float>,"VectorConstructData<float>")
// BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<int>,"VectorConstructData<int>")
// BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<double>,"VectorConstructData<double>")
// BOOST_CLASS_EXPORT_GUID(decaf::VectorConstructData<char>,"VectorConstructData<char>")

#endif
