#ifndef BASE_CONSTRUCT_DATA
#define BASE_CONSTRUCT_DATA

#include <bredala/data_model/basedata.h>
#include <bredala/data_model/block.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include "serialize_tuple.h"
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/unordered_map.hpp>

#include <boost/type_index.hpp>

#include <bredala/data_model/maptools.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <sstream>

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

namespace decaf {


class BaseConstructData {
public:

    /// @param map:		a container, map of fields with the field name as key and with additional information per field
    /// @param bCountable:      indicates whether this data type is countable or not

    BaseConstructData(mapConstruct map = mapConstruct(), bool bCountable = true) :
        map_(map), bCountable_(bCountable){}

    virtual ~BaseConstructData(){}

    //! serialization via boost
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(bCountable_);
    }

    virtual bool isCountable()
    {
        return bCountable_;
    }

    virtual void setCountable(bool bCountable)
    {
        bCountable_ = bCountable;
    }

    //! returns the number of semantic items
    virtual int getNbItems() = 0;

    //! returns the typename of a field
    virtual std::string getTypename() = 0;

    virtual bool isBlockSplitable() = 0;

    //! deprecated
    virtual bool appendItem(std::shared_ptr<BaseConstructData> dest, unsigned int index, ConstructTypeMergePolicy = DECAF_MERGE_DEFAULT) = 0;

    virtual void preallocMultiple(int nbCopies , int nbItems, std::vector<std::shared_ptr<BaseConstructData> >& result) = 0;

    virtual void softClean() = 0;

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT) = 0;

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT) = 0;

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< Block<3> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT) = 0;

    virtual void split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT) = 0;

    virtual void split(
            const std::vector< std::vector<int> >& range,
	    std::vector< mapConstruct >& partial_map,
	    std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT) = 0;

    virtual bool merge(std::shared_ptr<BaseConstructData> other,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT) = 0;

    virtual bool merge(std::vector<std::shared_ptr<BaseConstructData> >& others,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT) = 0;

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other) = 0;

    void setMap(mapConstruct map){ map_ = map; }
    mapConstruct getMap(){ return map_; }

protected:
    mapConstruct map_;  ///<  a container, map of fields with the field name as key and with additional information per field
    bool bCountable_;   ///<  indicates whether this data type is countable or not

};

}// namespace

#endif
