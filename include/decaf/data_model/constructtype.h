#ifndef CONSTRUCT_TYPE
#define CONSTRUCT_TYPE

#include <decaf/decaf.hpp>
#include <decaf/data_model/basedata.h>
#include <decaf/data_model/vectorconstructdata.hpp>

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

namespace decaf {

typedef std::tuple<ConstructTypeFlag, ConstructTypeScope,
    int, std::shared_ptr<BaseConstructData>,
    ConstructTypeSplitPolicy, ConstructTypeMergePolicy> datafield;

class ConstructData  : public BaseData {

public:
    ConstructData();

    virtual ~ConstructData(){}

    bool appendData(std::string name,
                    std::shared_ptr<BaseConstructData>  data,
                    ConstructTypeFlag flags = DECAF_NOFLAG,
                    ConstructTypeScope scope =  DECAF_PRIVATE,
                    ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,
                    ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT);

    int getNbFields();

    std::shared_ptr<std::map<std::string, datafield> > getMap();

    void printKeys();

    virtual bool hasZCurveKey();

    virtual const float* getZCurveKey(int *nbItems);

    virtual bool hasZCurveIndex();

    virtual const unsigned int* getZCurveIndex(int *nbItems);

    virtual bool isSplitable();

    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<int>& range);

    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<std::vector<int> >& range);

    //Todo : remove the code redundancy
    virtual bool merge(shared_ptr<BaseData> other);

    //Todo : remove the code redundancy
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

protected:
    std::shared_ptr<std::map<std::string, datafield> > container_;
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

    std::string out_serial_buffer_;
    std::string in_serial_buffer_;



};

} //namespace
BOOST_CLASS_EXPORT_GUID(decaf::ConstructData,"ConstructData")
#endif
