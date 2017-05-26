#ifndef CONSTRUCT_TYPE
#define CONSTRUCT_TYPE

//#include <decaf/decaf.hpp>
#include <bredala/data_model/basedata.h>
#include <bredala/data_model/vectorconstructdata.hpp>

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
#include <memory>


namespace decaf {

//Fields =
//   - Vector of the names of fields in the group
//   - Number of items in the group
//   - Has ZCurve Key (float*)
//   - Name of the ZCurve key
//   - Has ZCurve index (unsigned int*)
//   - Name of the ZCurve index

typedef std::tuple<std::vector<std::string>, int, bool, std::string, bool, std::string> groupField;

class ConstructData  : public BaseData {

public:
    ConstructData();

    virtual ~ConstructData(){}

    bool appendData(std::string name,
                    std::shared_ptr<BaseConstructData>  data,
                    ConstructTypeFlag flags = DECAF_NOFLAG,     // DECAF_NBITEMS, DECAF_ZCURVE_KEY
                    ConstructTypeScope scope =  DECAF_PRIVATE,  // DECAF_SHARED, DECAF_SYSTEM
                    ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,   // DECAF_SPLIT_KEEP_VALUE, ...
                    ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT,  // DECAF_MERGE_FIRST_VALUE, DECAF_MERGE_ADD_VALUE, ...
                    std::string groupName = "main");

    int getNbFields(std::string groupName = "");

    std::shared_ptr<std::map<std::string, datafield> > getMap();

    void printKeys();

    virtual bool hasZCurveKey();

    bool hasZCurveKey(std::string groupName);

    virtual const float* getZCurveKey(int *nbItems);

    const float* getZCurveKey(int *nbItems, std::string groupName);

    virtual bool hasZCurveIndex();

    bool hasZCurveIndex(std::string groupName);

    virtual const unsigned int* getZCurveIndex(int *nbItems);

    const unsigned int* getZCurveIndex(int *nbItems, std::string groupName);

    virtual bool isSplitable();

    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<int>& range);

    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<std::vector<int> >& range);

    virtual bool merge(std::shared_ptr<BaseData> other);

    virtual bool merge(char* buffer, int size);

    virtual bool merge();

    virtual bool serialize();

    virtual bool unserialize();

    //Prepare enough space in the serial buffer
    virtual void allocate_serial_buffer(int size);

    virtual char* getOutSerialBuffer(int* size);

    virtual char* getOutSerialBuffer();

    virtual int getOutSerialBufferSize();

    virtual char* getInSerialBuffer(int* size);

    virtual char* getInSerialBuffer();

    virtual int getInSerialBufferSize();

    virtual void purgeData();

    virtual bool setData(std::shared_ptr<void> data);

    std::shared_ptr<BaseConstructData> getData(std::string key);

    bool setMergeOrder(std::vector<std::string>& merge_order);
    const std::vector<std::string>& getMergeOrder();

    bool setSplitOrder(std::vector<std::string>& split_order);
    const std::vector<std::string>& getSplitOrder();

protected:
    mapConstruct container_;
    std::map<std::string,groupField> groups_;
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
    bool updateGroup(std::map<std::string, datafield>::iterator field, std::string groupName);

    std::string out_serial_buffer_;
    std::string in_serial_buffer_;

    std::vector<std::string> merge_order_;
    std::vector<std::string> split_order_;



};

} //namespace
BOOST_CLASS_EXPORT_GUID(decaf::ConstructData,"ConstructData")
#endif
