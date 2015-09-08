#ifndef ARRAY_CONSTRUCT_DATA
#define ARRAY_CONSTRUCT_DATA

#include <decaf/data_model/baseconstructdata.hpp>

namespace decaf {

template<typename T>
class ArrayConstructData : public BaseConstructData {
public:

    ArrayConstructData(mapConstruct map = mapConstruct()) : BaseConstructData(map){}

    ArrayConstructData(T* array, int size, int element_per_items, bool owner = false, mapConstruct map = mapConstruct()) :
                        value_(array), element_per_items_(element_per_items),
                        size_(size), owner_(owner), BaseConstructData(map){}

    virtual ~ArrayConstructData()
    {
        if(owner_) delete[] value_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(element_per_items_);
        ar & BOOST_SERIALIZATION_NVP(size_);
        ar & BOOST_SERIALIZATION_NVP(owner_);
        //ar & BOOST_SERIALIZATION_NVP(value_);
        ar & boost::serialization::make_array<T>(value_, size_);


    }

    virtual bool isBlockSplitable(){ return false; }

    virtual T* getArray(){ return value_; }

    virtual int getNbItems(){ return size_ / element_per_items_; }

    int getSize(){ return size_; }

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

                unsigned int offset = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    T* array = new T[range.at(i)*element_per_items_];
                    memcpy(array, value_ + offset, range.at(i)*element_per_items_ * sizeof(T));
                    std::shared_ptr<ArrayConstructData<T> > sub =
                            std::make_shared<ArrayConstructData<T> >(array, range.at(i)*element_per_items_, element_per_items_, true);
                    offset  += (range.at(i)*element_per_items_);
                    result.push_back(sub);
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                //The new subarray can't be the owner. The array will be deleted
                //when this is destroyed is this is the owner or the user will so it if
                //this is not the owner
                std::shared_ptr<ArrayConstructData> sub = std::make_shared<ArrayConstructData>(
                            value_, size_, element_per_items_, false);
                result.push_back(sub);
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for ArrayConstructData"<<std::endl;
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

                //typename std::vector<T>::iterator it = value_.begin();
                //unsigned int offset = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    T* array = new T[range.at(i).size() * element_per_items_];
                    unsigned int offset = 0;
                    for(unsigned int j = 0; j< range.at(i).size(); j++)
                    {
                        //temp.insert( temp.end(),
                        //             it+(range.at(i).at(j)*element_per_items_),
                        //             it+((range.at(i).at(j)+1)*element_per_items_)
                        //             );
                        memcpy(array + offset,
                               value_ + range.at(i).at(j)*element_per_items_,
                               element_per_items_ * sizeof(T));
                        offset += element_per_items_;
                    }

                    std::shared_ptr<ArrayConstructData<T> > sub =
                            std::make_shared<ArrayConstructData<T> >(array, range.at(i).size() * element_per_items_,
                                                                     element_per_items_, true);
                    result.push_back(sub);
                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for ArrayConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    //This function should never be called
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
        std::shared_ptr<ArrayConstructData<T> > other_ = std::dynamic_pointer_cast<ArrayConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                if(size_ != other_->size_)
                {
                    std::cout<<"ERROR : Trying to apply default merge policy with ArrayConstructData"
                             <<" Default policy keep one array and check that the second is identical."
                             <<" The 2 arrays have different sizes."<<std::endl;
                    return false;
                }
                for(int i = 0; i < size_; i++)
                {
                    if(value_ != other_->value_)
                    {
                        std::cout<<"ERROR : The original and other data do not have the same data."
                        <<"Default policy keep one array and check that the 2 merged values are"
                        <<" the same. Make sure the values are the same or change the merge policy"<<std::endl;
                        return false;
                    }
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
                std::cout<<"Merging arrays of size "<<size_<<" and "<<other_->size_<<std::endl;
                T* newArray = new T[size_ + other_->size_];
                memcpy(newArray, value_, size_ * sizeof(T));
                memcpy(newArray + size_, other_->value_, other_->size_);

                if(owner_) delete[] value_;
                value_ = newArray;
                size_ = size_ + other_->size_;
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
        std::shared_ptr<ArrayConstructData<T> >other_ = std::dynamic_pointer_cast<ArrayConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }



protected:
    T* value_;
    int element_per_items_; //One semantic item is composed of element_per_items_ items in the vector
    int size_;
    bool owner_;
};

} //namespace

BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<float>,"ArrayConstructData<float>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<int>,"ArrayConstructData<int>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<double>,"ArrayConstructData<double>")
BOOST_CLASS_EXPORT_GUID(decaf::ArrayConstructData<char>,"ArrayConstructData<char>")

#endif

