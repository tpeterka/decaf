#ifndef CONSTRUCT_TYPE
#define CONSTRUCT_TYPE

#include <decaf/decaf.hpp>
#include <decaf/transport/mpi/newData.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include "serialize_tuple.h"
//#include <boost/serialization/shared_ptr.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/unordered_map.hpp>



#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <sstream>

using namespace decaf;

enum ConstructTypeFlag {
    DECAF_NOFLAG = 0x0,     // No specific information on the data field
    DECAF_NBITEM = 0x1,     // Field represents the number of item in the collection
    DECAF_ZCURVEKEY = 0x2,  // Field that can be used as a key for the ZCurve (position)
    DECAF_ZCURVEINDEX = 0x4 // Field that can be used as the index for the ZCurve (hilbert code)
};

enum ConstructTypeScope {
    DECAF_SHARED = 0x0,     // This value is the same for all the items in the collection
    DECAF_PRIVATE = 0x1,    // Different values for each items in the collection
};

enum ConstructTypeSplitPolicy {
    DECAF_SPLIT_DEFAULT = 0x0,      // Call the split fonction of the data object
    DECAF_SPLIT_KEEP_VALUE = 0x1,   // Keep the same values for each split
    DECAF_SPLIT_MINUS_NBITEM = 0x2, // Withdraw the number of items to the current value
};

enum ConstructTypeMergePolicy {
    DECAF_MERGE_DEFAULT = 0x0,        // Call the split fonction of the data object
    DECAF_MERGE_FIRST_VALUE = 0x1,    // Keep the same values for each split
    DECAF_MERGE_ADD_VALUE = 0x2,      // Add the values
    DECAF_MERGE_APPEND_VALUES = 0x4,  // Append the values into the current object
};

//---/ Wrapper for std::shared_ptr<> /------------------------------------------

namespace boost { namespace serialization {

template<class Archive, class Type>
void save(Archive & archive, const std::shared_ptr<Type> & value, const unsigned int /*version*/)
{
    Type *data = value.get();
    archive << data;
}

template<class Archive, class Type>
void load(Archive & archive, std::shared_ptr<Type> & value, const unsigned int /*version*/)
{
    Type *data;
    archive >> data;

    typedef std::weak_ptr<Type> WeakPtr;
    static boost::unordered_map<void*, WeakPtr> hash;

    if (hash[data].expired())
    {
         value = std::shared_ptr<Type>(data);
         hash[data] = value;
    }
    else value = hash[data].lock();
}

template<class Archive, class Type>
inline void serialize(Archive & archive, std::shared_ptr<Type> & value, const unsigned int version)
{
    split_free(archive, value, version);
}

}}



class BaseConstructData {
public:
    BaseConstructData(){}
    virtual ~BaseConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    }

    virtual int getNbItems() = 0;

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT ) = 0;

    virtual bool merge(std::shared_ptr<BaseConstructData> other,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT) = 0;

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other) = 0;

};
BOOST_CLASS_EXPORT_GUID(BaseConstructData,"BaseConstructData")

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

BOOST_CLASS_EXPORT_GUID(SimpleConstructData<float>,"SimpleConstructData<float>")
BOOST_CLASS_EXPORT_GUID(SimpleConstructData<int>,"SimpleConstructData<int>")
BOOST_CLASS_EXPORT_GUID(SimpleConstructData<double>,"SimpleConstructData<double>")
BOOST_CLASS_EXPORT_GUID(SimpleConstructData<char>,"SimpleConstructData<char>")


template<typename T>
class ArrayConstructData : public BaseConstructData {
public:
    ArrayConstructData() : BaseConstructData(){}
    ArrayConstructData(std::vector<T>& value, int element_per_items) :
                       BaseConstructData(), value_(value),
                       element_per_items_(element_per_items){}
    ArrayConstructData(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end,
                       int element_per_items) : element_per_items_(element_per_items)
    {
        value_ = std::vector<T>(begin, end);
    }
    ArrayConstructData(T* array, int size, int element_per_items) :
                        element_per_items_(element_per_items)
    {
        value_.resize(size);
        value_ = std::vector<T>(array, array + size);
    }

    virtual ~ArrayConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
        ar & BOOST_SERIALIZATION_NVP(element_per_items_);
    }

    std::vector<T>& getArray(){ return value_; }

    virtual int getNbItems(){ return value_.size() / element_per_items_; }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
          const std::vector<int>& range,
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
                    std::shared_ptr<ArrayConstructData<T> > sub =
                            std::make_shared<ArrayConstructData<T> >(it, it+(range.at(i)*element_per_items_), element_per_items_);
                    it  = it+(range.at(i)*element_per_items_);
                    result.push_back(sub);
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                std::shared_ptr<ArrayConstructData> sub = std::make_shared<ArrayConstructData>(
                            value_.begin(), value_.end(), element_per_items_);
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

    virtual bool merge(std::shared_ptr<BaseConstructData> other,
              ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<ArrayConstructData<T> > other_ = dynamic_pointer_cast<ArrayConstructData<T> >(other);
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
        std::shared_ptr<ArrayConstructData<T> >other_ = dynamic_pointer_cast<ArrayConstructData<T> >(other);
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
BOOST_CLASS_EXPORT_GUID(ArrayConstructData<float>,"ArrayConstructData<float>")
BOOST_CLASS_EXPORT_GUID(ArrayConstructData<int>,"ArrayConstructData<int>")
BOOST_CLASS_EXPORT_GUID(ArrayConstructData<double>,"ArrayConstructData<double>")
BOOST_CLASS_EXPORT_GUID(ArrayConstructData<char>,"ArrayConstructData<char>")

typedef std::tuple<ConstructTypeFlag, ConstructTypeScope,
    int, std::shared_ptr<BaseConstructData>,
    ConstructTypeSplitPolicy, ConstructTypeMergePolicy> datafield;

class ConstructData  : public BaseData {

public:
    ConstructData() : BaseData(), nbFields_(0), bZCurveIndex_(false), zCurveIndex_(NULL),
        bZCurveKey_(false), zCurveKey_(NULL)
    {
        container_ = std::make_shared<std::map<std::string, datafield> >();
        data_ = static_pointer_cast<void>(container_);
    }
    virtual ~ConstructData(){}

    bool appendData(std::string name,
                    std::shared_ptr<BaseConstructData>  data,
                    ConstructTypeFlag flags = DECAF_NOFLAG,
                    ConstructTypeScope scope =  DECAF_PRIVATE,
                    ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,
                    ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT)
    {
        std::pair<std::map<std::string, datafield>::iterator,bool> ret;
        datafield newEntry = make_tuple(flags, scope, data->getNbItems(), data, splitFlag, mergeFlag);
        ret = container_->insert(std::pair<std::string, datafield>(name, newEntry));

        return ret.second && updateMetaData();
    }

    //int getNbItems(){ return nbItems_; }
    int getNbFields(){ return nbFields_; }
    std::shared_ptr<std::map<std::string, datafield> > getMap(){ return container_; }
    //std::map<std::string, datafield>* getMap(){ return container_; }

    void printKeys()
    {
        std::cout<<"Current state of the map : "<<std::endl;
        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            std::cout<<"Key : "<<it->first<<", nbItems : "<<std::get<2>(it->second)<<std::endl;
        }
        std::cout<<"End of display of the map"<<std::endl;

    }

    virtual bool hasZCurveKey(){ return bZCurveKey_; }

    virtual const float* getZCurveKey(int *nbItems)
    {
        if(!bZCurveKey_ || zCurveKey_)
            return NULL;

        std::shared_ptr<ArrayConstructData<float> > field =
                dynamic_pointer_cast<ArrayConstructData<float> >(zCurveKey_);
        if(!field){
            std::cout<<"ERROR : The field with the ZCURVE flag is not of type ArrayConstructData<float>"<<std::endl;
            return NULL;
        }

        *nbItems = field->getNbItems();
        return &(field->getArray()[0]);
    }

    virtual bool hasZCurveIndex(){ return bZCurveIndex_; }

    virtual const unsigned int* getZCurveIndex(int *nbItems)
    {
        if(!bZCurveIndex_ || zCurveIndex_)
            return NULL;

        std::shared_ptr<ArrayConstructData<unsigned int> > field =
                dynamic_pointer_cast<ArrayConstructData<unsigned int> >(zCurveIndex_);
        if(!field){
            std::cout<<"ERROR : The field with the ZCURVE flag is not of type ArrayConstructData<float>"<<std::endl;
            return NULL;
        }

        *nbItems = field->getNbItems();
        return &(field->getArray()[0]);
    }

    virtual bool isSplitable(){ return nbItems_ > 1; }

    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<int>& range)
    {
        std::vector< std::shared_ptr<BaseData> > result;
        for(unsigned int i = 0; i < range.size(); i++)
            result.push_back(std::make_shared<ConstructData>());

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

        for(std::map<std::string, datafield>::iterator it = container_->begin();
            it != container_->end(); it++)
        {
            // Splitting the current field
            std::vector<std::shared_ptr<BaseConstructData> > splitFields;
            splitFields = std::get<3>(it->second)->split(range, std::get<4>(it->second));

            // Inserting the splitted field into the splitted results
            if(splitFields.size() != result.size())
            {
                std::cout<<"ERROR : A field was not splited properly."
                        <<" The number of chunks does not match the expected number of chunks"<<std::endl;
                // Cleaning the result to avoid corrupt data
                result.clear();

                return result;
            }

            //Adding the splitted results into the splitted maps
            for(unsigned int i = 0; i < result.size(); i++)
            {
                std::shared_ptr<ConstructData> construct = dynamic_pointer_cast<ConstructData>(result.at(i));
                construct->appendData(it->first,
                                         splitFields.at(i),
                                         std::get<0>(it->second),
                                         std::get<1>(it->second),
                                         std::get<4>(it->second),
                                         std::get<5>(it->second)
                                         );
            }
        }

        return result;
    }

    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<std::vector<int> >& range)
    {
        std::vector< std::shared_ptr<BaseData> > result;

        return result;
    }
    //Todo : remove the code redundancy
    virtual bool merge(shared_ptr<BaseData> other)
    {
        std::shared_ptr<ConstructData> otherConstruct = std::dynamic_pointer_cast<ConstructData>(other);
        if(!otherConstruct)
        {
            std::cout<<"ERROR : Trying to merge two objects which have not the same dynamic type"<<std::endl;
            return false;
        }

        //No data yet, we simply copy the data from the other map
        if(!data_ || container_->empty())
        {
            //TODO : DANGEROUS should use a copy function
            container_ = otherConstruct->container_;
            data_ = static_pointer_cast<void>(container_);
            nbItems_ = otherConstruct->nbItems_;
            nbFields_ = otherConstruct->nbFields_;
            bZCurveKey_ = otherConstruct->bZCurveKey_;
            zCurveKey_ = otherConstruct->zCurveKey_;
            bZCurveIndex_ = otherConstruct->bZCurveIndex_;
            zCurveIndex_ = otherConstruct->zCurveIndex_;
        }
        else
        {
            //We check that we can merge all the fields before merging
            if(container_->size() != otherConstruct->getMap()->size())
            {
                std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
                return false;
            }

            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                std::map<std::string, datafield>::iterator otherIt = otherConstruct->getMap()->find(it->first);
                if( otherIt == otherConstruct->getMap()->end())
                {
                    std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
                             <<"In the original map but not in the other one. Merge aborted."<<std::endl;
                    return false;
                }
                if( !std::get<3>(otherIt->second)->canMerge(std::get<3>(it->second)) )
                    return false;
            }

            //TODO : Add a checking on the merge policy and number of items in case of a merge

            //We have done all the checking, now we can merge securely
            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                std::map<std::string, datafield>::iterator otherIt = otherConstruct->getMap()->find(it->first);

                if(! std::get<3>(it->second)->merge(std::get<3>(otherIt->second), std::get<5>(otherIt->second)) )
                {
                    std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
                    return false;
                }
                std::get<2>(it->second) = std::get<3>(it->second)->getNbItems();
            }
        }

        return updateMetaData();
    }

    //Todo : remove the code redundancy
    virtual bool merge(char* buffer, int size)
    {
        //serial_buffer_.getline(buffer, size);
        //boost::archive::binary_iarchive ia(serial_buffer_);
        in_serial_buffer_ = std::string(buffer, size);
        std::cout<<"Serial buffer size : "<<in_serial_buffer_.size()<<std::endl;
        boost::iostreams::basic_array_source<char> device(in_serial_buffer_.data(), in_serial_buffer_.size());
        boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
        boost::archive::binary_iarchive ia(sout);

        fflush(stdout);

        if(!data_ || container_->empty())
        {
            ia >> container_;

            data_ = std::static_pointer_cast<void>(container_);

            /*nbFields_ = container_->size();
            splitable_ = false;
            bZCurveKey_ = false;
            bZCurveIndex_ = false;

            //Going through the map to check the meta data
            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                if(nbItems_ > 1 && std::get<2>(it->second) > 1 && nbItems_ != std::get<2>(it->second))
                {
                    std::cout<<"ERROR : can add new field with "<<std::get<2>(it->second)<<" items."
                            <<" The current map has "<<nbItems_<<" items. The number of items "
                            <<"of the new filed should be 1 or "<<nbItems_<<std::endl;
                    return false;
                }
                else if(nbItems_ == 0 || std::get<2>(it->second) > 1)// We still update the number of items
                    nbItems_ = std::get<2>(it->second);

                if(std::get<0>(it->second) == DECAF_ZCURVEKEY)
                {
                    bZCurveKey_ = true;
                    zCurveKey_ = std::get<3>(it->second);
                }

                if(std::get<0>(it->second) == DECAF_ZCURVEINDEX)
                {
                    bZCurveIndex_ = true;
                    zCurveIndex_ = std::get<3>(it->second);
                }
            }*/
        }
        else
        {
            std::shared_ptr<std::map<std::string, datafield> > other;
            ia >> other;

            //We check that we can merge all the fields before merging
            if(container_->size() != other->size())
            {
                std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
                return false;
            }

            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                std::map<std::string, datafield>::iterator otherIt = other->find(it->first);
                if( otherIt == other->end())
                {
                    std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
                             <<"In the original map but not in the other one. Merge aborted."<<std::endl;
                    return false;
                }
                if( !std::get<3>(otherIt->second)->canMerge(std::get<3>(it->second)))
                    return false;
            }

            //TODO : Add a checking on the merge policy and number of items in case of a merge

            //We have done all the checking, now we can merge securely
            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                std::map<std::string, datafield>::iterator otherIt = other->find(it->first);

                if(! std::get<3>(it->second)->merge(std::get<3>(otherIt->second),
                                                    std::get<5>(otherIt->second)) )
                {
                    std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
                    return false;
                }
                std::get<2>(it->second) = std::get<3>(it->second)->getNbItems();
            }
        }

        return updateMetaData();
    }

    virtual bool merge()
    {
        std::cout<<"Size of the received string : "<<in_serial_buffer_.size()<<std::endl;
        boost::iostreams::basic_array_source<char> device(in_serial_buffer_.data(), in_serial_buffer_.size());
        boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
        boost::archive::binary_iarchive ia(sout);

        if(!data_ || container_->empty())
        {
            ia >> container_;

            data_ = std::static_pointer_cast<void>(container_);

            /*nbFields_ = container_->size();
            splitable_ = false;
            bZCurveKey_ = false;
            bZCurveIndex_ = false;

            //Going through the map to check the meta data
            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                if(nbItems_ > 1 && std::get<2>(it->second) > 1 && nbItems_ != std::get<2>(it->second))
                {
                    std::cout<<"ERROR : can add new field with "<<std::get<2>(it->second)<<" items."
                            <<" The current map has "<<nbItems_<<" items. The number of items "
                            <<"of the new filed should be 1 or "<<nbItems_<<std::endl;
                    return false;
                }
                else if(nbItems_ == 0 || std::get<2>(it->second) > 1)// We still update the number of items
                    nbItems_ = std::get<2>(it->second);
            }*/
        }
        else
        {
            std::shared_ptr<std::map<std::string, datafield> > other;
            ia >> other;

            //We check that we can merge all the fields before merging
            if(container_->size() != other->size())
            {
                std::cout<<"Error : the map don't have the same number of field. Merge aborted."<<std::endl;
                return false;
            }

            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                std::map<std::string, datafield>::iterator otherIt = other->find(it->first);
                if( otherIt == other->end())
                {
                    std::cout<<"Error : The field \""<<it->first<<"\" is present in the"
                             <<"In the original map but not in the other one. Merge aborted."<<std::endl;
                    return false;
                }
                if( !std::get<3>(otherIt->second)->canMerge(std::get<3>(it->second)) )
                    return false;
            }

            //TODO : Add a checking on the merge policy and number of items in case of a merge

            //We have done all the checking, now we can merge securely
            for(std::map<std::string, datafield>::iterator it = container_->begin();
                it != container_->end(); it++)
            {
                std::map<std::string, datafield>::iterator otherIt = other->find(it->first);

                if(! std::get<3>(it->second)->merge(std::get<3>(otherIt->second),
                                                    std::get<5>(otherIt->second)) )
                {
                    std::cout<<"Error while merging the field \""<<it->first<<"\". The original map has be corrupted."<<std::endl;
                    return false;
                }
                std::get<2>(it->second) = std::get<3>(it->second)->getNbItems();
            }
        }

        return updateMetaData();
    }

    virtual bool serialize()
    {
        //boost::archive::binary_oarchive oa(serial_buffer_);
        boost::iostreams::back_insert_device<std::string> inserter(out_serial_buffer_);
        boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
        boost::archive::binary_oarchive oa(s);

        oa << container_;
        s.flush();

        std::cout<<"Size of the string after serialization : "<<out_serial_buffer_.size()<<std::endl;

        std::cout<<"Test serialization : "<<std::endl;
        std::string test_deserialize;
        test_deserialize.resize(out_serial_buffer_.size());
        memcpy(&test_deserialize[0], &out_serial_buffer_[0], out_serial_buffer_.size());
        boost::iostreams::basic_array_source<char> device(test_deserialize.data(), test_deserialize.size());
        boost::iostreams::stream<boost::iostreams::basic_array_source<char> > sout(device);
        boost::archive::binary_iarchive ia(sout);
        std::cout<<"Serialization test completed."<<std::endl;
        return true;
    }

    virtual bool unserialize()
    {
        return false;
    }

    //Prepare enough space in the serial buffer
    virtual void allocate_serial_buffer(int size)
    {
        std::cout<<"Allocating for "<<size<<" bytes"<<std::endl;
        in_serial_buffer_.resize(size);
    }

    virtual char* getOutSerialBuffer(int* size)
    {
        *size = out_serial_buffer_.size(); //+1 for the \n caractere
        return &out_serial_buffer_[0]; //Dangerous if the string gets reallocated
    }
    virtual char* getOutSerialBuffer()
    {
        std::cout<<"Out serial called"<<std::endl;
        char* buffer = &out_serial_buffer_[0];
        std::cout<<"Adress : "<<&buffer<<std::endl;
        return &out_serial_buffer_[0];
    }
    virtual int getOutSerialBufferSize(){ return out_serial_buffer_.size();}

    virtual char* getInSerialBuffer(int* size)
    {
        *size = in_serial_buffer_.size(); //+1 for the \n caractere
        return &in_serial_buffer_[0]; //Dangerous if the string gets reallocated
    }
    virtual char* getInSerialBuffer()
    {
        std::cout<<"In serial called"<<std::endl;
        char* buffer = &in_serial_buffer_[0];
        std::cout<<"Adress : "<<&buffer<<std::endl;
        return &in_serial_buffer_[0];
    }
    virtual int getInSerialBufferSize(){ return in_serial_buffer_.size(); }

    /*virtual char* getSerialBuffer(int* size)
    {
        *size = serial_buffer_.str().size();
        return &(serial_buffer_.str()[0]);
    }
    virtual char* getSerialBuffer(){ return &(serial_buffer_.str()[0]); }
    virtual int getSerialBufferSize(){ return serial_buffer_.str().size(); }*/

    /*virtual char* getSerialBuffer(int* size)
    {
        *size = serial_buffer_.size(); //+1 for the \n caractere
        return &(serial_buffer_[0]); //Dangerous if the string gets reallocated
    }
    virtual char* getSerialBuffer(){ return &(serial_buffer_[0]); }
    virtual int getSerialBufferSize(){ return serial_buffer_.size(); }*/

    virtual void purgeData()
    {
        //To purge the data we just have to clean the map and reset the metadatas
        container_->clear();
        nbFields_ = 0;
        nbItems_ = 0;
        bZCurveKey_ = false;
        bZCurveIndex_ = false;
        zCurveKey_.reset();
        zCurveIndex_.reset();
        splitable_ = false;
    }

    virtual bool setData(std::shared_ptr<void> data)
    {
        std::shared_ptr<std::map<std::string, datafield> > container =
                static_pointer_cast<std::map<std::string, datafield> >(data);

        if(!container){
            std::cout<<"ERROR : can not cast the data into the proper type."<<std::endl;
            return false;
        }

        //Checking is the map is valid and updating the informations
        int nbItems = 0, nbFields = 0;
        bool bZCurveKey = false, bZCurveIndex = false;
        std::shared_ptr<BaseConstructData> zCurveKey, zCurveIndex;
        for(std::map<std::string, datafield>::iterator it = container->begin();
            it != container->end(); it++)
        {
            // Checking that we can insert this data and keep spliting the data after
            // If we already have fields with several items and we insert a new field
            // with another number of items, we can't split automatically
            if(nbItems > 1 && nbItems != std::get<2>(it->second))
            {
                std::cout<<"ERROR : can add new field with "<<std::get<2>(it->second)<<" items."
                        <<" The current map has "<<nbItems<<" items. The number of items "
                        <<"of the new filed should be 1 or "<<nbItems<<std::endl;
                return false;
            }
            else // We still update the number of items
                nbItems_ = std::get<2>(it->second);

            if(std::get<0>(it->second) == DECAF_ZCURVEKEY)
            {
                bZCurveKey = true;
                zCurveKey = std::get<3>(it->second);
            }

            if(std::get<0>(it->second) == DECAF_ZCURVEINDEX)
            {
                bZCurveIndex = true;
                zCurveIndex = std::get<3>(it->second);
            }

            //The field is already in the map, we don't have to test the insert
            nbFields++;
        }

        // We have checked all the fields without issue, we can use this map
        //container_ = container.get();
        container_ = container;
        nbItems_ = nbItems;
        nbFields_ = nbFields;
        bZCurveKey_ = bZCurveKey;
        bZCurveIndex_ = bZCurveIndex;
        zCurveKey_ = zCurveKey;
        zCurveIndex_ = zCurveIndex;

        return true;

    }

    std::shared_ptr<BaseConstructData> getData(std::string key)
    {
        std::map<std::string, datafield>::iterator it;
        it = container_->find(key);
        if(it == container_->end())
        {
            std::cout<<"ERROR : key "<<key<<" not found."<<std::endl;
            return std::shared_ptr<BaseConstructData>();
        }
        else
            return std::get<3>(it->second);
    }

    //std::stringstream getStringStream(){ return serial_buffer_;}

protected:
    std::shared_ptr<std::map<std::string, datafield> > container_;
    //std::map<std::string, datafield> *container_;
    //int nbItems_;
    int nbFields_;
    bool bZCurveKey_;
    bool bZCurveIndex_;
    std::shared_ptr<BaseConstructData> zCurveKey_;
    std::shared_ptr<BaseConstructData> zCurveIndex_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(container_);
        ar & BOOST_SERIALIZATION_NVP(nbItems_);
    }

    bool updateMetaData();

   //std::stringstream serial_buffer_;
    std::string out_serial_buffer_;
    std::string in_serial_buffer_;
    //std::string serial_buffer_;



};
BOOST_CLASS_EXPORT_GUID(ConstructData,"ConstructData")

bool ConstructData::updateMetaData()
{
    //Checking is the map is valid and updating the informations
    nbItems_ = 0;
    nbFields_ = 0;
    bZCurveKey_ = false;
    bZCurveIndex_ = false;

    for(std::map<std::string, datafield>::iterator it = container_->begin();
        it != container_->end(); it++)
    {
        // Checking that we can insert this data and keep spliting the data after
        // If we already have fields with several items and we insert a new field
        // with another number of items, we can't split automatically
        if(nbItems_ > 1 && nbItems_ != std::get<2>(it->second))
        {
            std::cout<<"ERROR : can add new field with "<<std::get<2>(it->second)<<" items."
                    <<" The current map has "<<nbItems_<<" items. The number of items "
                    <<"of the new filed should be 1 or "<<nbItems_<<std::endl;
            return false;
        }
        else // We still update the number of items
            nbItems_ = std::get<2>(it->second);

        if(std::get<0>(it->second) == DECAF_ZCURVEKEY)
        {
            bZCurveKey_ = true;
            zCurveKey_ = std::get<3>(it->second);
        }

        if(std::get<0>(it->second) == DECAF_ZCURVEINDEX)
        {
            bZCurveIndex_ = true;
            zCurveIndex_ = std::get<3>(it->second);
        }
        //The field is already in the map, we don't have to test the insert
        nbFields_++;
    }

    return true;
}

#endif
