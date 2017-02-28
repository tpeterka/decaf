#ifndef VECTOR_CONSTRUCT_DATA
#define VECTOR_CONSTRUCT_DATA

#include "baseconstructdata.hpp"

namespace decaf {

template<typename T>
class VectorConstructData : public BaseConstructData {
public:
    VectorConstructData(mapConstruct map = mapConstruct(), bool bCountable = true)
        : BaseConstructData(map, bCountable){}

    VectorConstructData(std::vector<T>& value, int element_per_items, mapConstruct map = mapConstruct(), bool bCountable = true) :
                       BaseConstructData(map, bCountable), value_(value),
                       element_per_items_(element_per_items){}
    VectorConstructData(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end,
                       int element_per_items, mapConstruct map = mapConstruct(), bool bCountable = true) :
        element_per_items_(element_per_items), BaseConstructData(map, bCountable)
    {
        value_ = std::vector<T>(begin, end);
    }
    VectorConstructData(T* Vector, int size, int element_per_items, mapConstruct map = mapConstruct(), bool bCountable = true) :
                        element_per_items_(element_per_items), BaseConstructData(map, bCountable)
    {
        value_.resize(size);
        value_ = std::vector<T>(Vector, Vector + size);
    }

    VectorConstructData(int element_per_items, int capacity, mapConstruct map = mapConstruct(), bool bCountable = true) :
                        element_per_items_(element_per_items), BaseConstructData(map, bCountable)
    {
        value_.reserve(capacity * element_per_items_);
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

    virtual bool appendItem(std::shared_ptr<BaseConstructData> dest, unsigned int index, ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
	std::shared_ptr<VectorConstructData<T> > destT = std::dynamic_pointer_cast<VectorConstructData<T> >(dest);
        if(!destT)
        {
            std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
            return false;
        }
	
	switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
		destT->value_.insert(destT->value_.end(),
				     value_.begin() + (index * element_per_items_), 
				     value_.begin() + ((index + 1 ) * element_per_items_));
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
		destT->value_.insert(destT->value_.end(), 
                                     value_.begin() + (index * element_per_items_), 
                                     value_.begin() + ((index + 1) * element_per_items_));
                return true;
                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for vector data."<<std::endl;
                return false;
                break;
            }
        }
        return false;
    }

    virtual void preallocMultiple(int nbCopies , int nbItems, std::vector<std::shared_ptr<BaseConstructData> >& result)
    {
        for(unsigned int i = 0; i < nbCopies; i++)
        {
                result.push_back(std::make_shared<VectorConstructData>(element_per_items_, nbItems, getMap()));
        }
    }

    virtual std::vector<T>& getVector(){ return value_; }

    virtual int getNbItems(){ return value_.size() / element_per_items_; }

	virtual std::string getTypename(){ return std::string("Vector_" + boost::typeindex::type_id<T>().pretty_name()); }

    virtual void split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT:
            {

                typename std::vector<T>::iterator it = value_.begin();
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<VectorConstructData<T> > sub = std::dynamic_pointer_cast<VectorConstructData<T> >(fields[i]);
                    assert(sub);
                    sub->getVector().reserve(range[i].back()*element_per_items_);


                    for(unsigned int j = 0; j < range[i].size()-1; j+=2) //The last number is the total nb of items
                    {
                        sub->getVector().insert(
                                    sub->getVector().end(),
                                    it+(range[i][j]*element_per_items_),
                                    it+(range[i][j]*element_per_items_)+(range[i][j+1]*element_per_items_));
                    }

                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for VectorConstructData"<<std::endl;
                break;
            }
        }
    }

    virtual void split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {

        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range.at(i);
                if(totalRange > getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return;
                }

                typename std::vector<T>::iterator it = value_.begin();
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<VectorConstructData<T> > sub = std::dynamic_pointer_cast<VectorConstructData<T> >(fields[i]);
                    assert(sub);
                    sub->getVector().insert(sub->getVector().begin(), it, it+(range.at(i)*element_per_items_));
                    it  = it+(range.at(i)*element_per_items_);
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<VectorConstructData<T> > sub = std::dynamic_pointer_cast<VectorConstructData<T> >(fields[i]);
                    assert(sub);
                    sub->getVector().insert(sub->getVector().begin(), value_.begin(), value_.end());
                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for VectorConstructData"<<std::endl;
                break;
            }
        }

        return;
    }

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
        std::cout<<"ERROR : calling a split on block."<<std::endl;
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
                value_.insert(value_.end(), other_->value_.begin(), other_->value_.end());
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
                std::cout<<"ERROR : policy "<<policy<<" not available for vector data."<<std::endl;
                return false;
                break;
            }
        }
    }

    virtual bool merge(std::vector<std::shared_ptr<BaseConstructData> >& others,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                for(unsigned int i = 0; i < others.size(); i++)
                {
                    std::shared_ptr<VectorConstructData<T> > other_ = std::dynamic_pointer_cast<VectorConstructData<T> >(others[i]);
                    if(!other_)
                    {
                        std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
                        return false;
                    }

                    value_.insert(value_.end(), other_->value_.begin(), other_->value_.end());

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
                for(unsigned int i = 0; i < others.size(); i++)
                {
                    std::shared_ptr<VectorConstructData<T> > other_ = std::dynamic_pointer_cast<VectorConstructData<T> >(others[i]);
                    if(!other_)
                    {
                        std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
                        return false;
                    }

                    value_.insert(value_.end(), other_->value_.begin(), other_->value_.end());

                }

                return true;
                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for vector data."<<std::endl;
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

    virtual void softClean()
    {
	value_.clear();
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
