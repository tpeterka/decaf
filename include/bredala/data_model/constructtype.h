#ifndef CONSTRUCT_TYPE
#define CONSTRUCT_TYPE

//#include <decaf/decaf.hpp>
#include <bredala/data_model/basedata.h>
#include <bredala/data_model/vectorconstructdata.hpp>
#include <bredala/data_model/block.hpp>
#include <bredala/data_model/basefield.hpp>

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

#include <bredala/data_model/maptools.h>

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <sstream>
#include <memory>
#include <string>


//extern double timeGlobalSerialization;
//extern double timeGlobalSplit;
//extern double timeGlobalMorton;

extern double timeGlobalSplit;
extern double timeGlobalBuild;
extern double timeGlobalMerge;
extern double timeGlobalRedist;
extern double timeGlobalReceiv;

namespace decaf {

class pConstructData;

class ConstructData  : public BaseData {

public:
	ConstructData();

	virtual ~ConstructData(){}

        //! appends the data field (which has associated flags for its type, scope, split and merge policies) to the data model
	bool appendData(std::string name,
	                std::shared_ptr<BaseConstructData>  data,
	                ConstructTypeFlag flags = DECAF_NOFLAG,     // DECAF_NBITEMS, DECAF_ZCURVE_KEY
	                ConstructTypeScope scope =  DECAF_PRIVATE,  // DECAF_SHARED, DECAF_SYSTEM
	                ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,   // DECAF_SPLIT_KEEP_VALUE, ...
	                ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT);  // DECAF_MERGE_FIRST_VALUE, DECAF_MERGE_ADD_VALUE, ...

	bool appendData(std::string name,
	                BaseField&  data,
	                ConstructTypeFlag flags = DECAF_NOFLAG,     // DECAF_NBITEMS, DECAF_ZCURVE_KEY
	                ConstructTypeScope scope =  DECAF_PRIVATE,  // DECAF_SHARED, DECAF_SYSTEM
	                ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,   // DECAF_SPLIT_KEEP_VALUE, ...
	                ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT);  // DECAF_MERGE_FIRST_VALUE, DECAF_MERGE_ADD_VALUE, ...
	bool appendData(const char* name,
	                std::shared_ptr<BaseConstructData>  data,
	                ConstructTypeFlag flags = DECAF_NOFLAG,
	                ConstructTypeScope scope =  DECAF_PRIVATE,
	                ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,
	                ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT);

	bool appendData(const char* name,
	                BaseField& data,
	                ConstructTypeFlag flags = DECAF_NOFLAG,
	                ConstructTypeScope scope =  DECAF_PRIVATE,
	                ConstructTypeSplitPolicy splitFlag = DECAF_SPLIT_DEFAULT,
	                ConstructTypeMergePolicy mergeFlag = DECAF_MERGE_DEFAULT);

	// Append the field of given name taken from the container data
	bool appendData(pConstructData data, const std::string name);
	bool appendData(pConstructData data, const char* name);

	//! deprecated
	bool appendItem(std::shared_ptr<ConstructData> dest, unsigned int index);

	//void preallocMultiple(int nbCopies , int nbItems, std::vector<pConstructData >& result);

	bool removeData(std::string name);

	bool updateData(std::string name,
	                std::shared_ptr<BaseConstructData>  data);

	int getNbFields();

	//! returns true if name is present and type is the typename of the field
	bool getTypename(std::string &name, std::string &type);

	//! deprecated
	bool isCoherent();

	//! returns the container (collection of the fields)
	std::shared_ptr<std::map<std::string, datafield> > getMap();

	void printKeys();

	std::vector<std::string> listUserKeys();

	virtual bool hasZCurveKey();

	virtual const float* getZCurveKey(int *nbItems);

	virtual bool hasZCurveIndex();

	virtual const unsigned int* getZCurveIndex(int *nbItems);

	virtual bool isSplitable();

	virtual bool isSystem();

	virtual void setSystem(bool bSystem);

	virtual bool hasSystem();

	virtual bool isEmpty();

    virtual void setToken(bool bToken);

    virtual bool isToken();

	bool isCountable(); ///< returns true when the data model is countable/splitable

	bool isPartiallyCountable();

	// Inherited from baseconstruct
	virtual std::vector< std::shared_ptr<BaseData> > split(
	        const std::vector<int>& range);

	// Version to call for buffering
	virtual void split(
	        const std::vector<int>& range,
	        std::vector<pConstructData > buffers);

	virtual std::vector< std::shared_ptr<BaseData> > split(
	        const std::vector<std::vector<int> >& range);

	virtual void split(
	        const std::vector<std::vector<int> >& range,
	        std::vector<pConstructData > buffers);

	virtual std::vector< std::shared_ptr<BaseData> > split(
	        const std::vector<Block<3> >& range);

	virtual void split(
	        const std::vector<Block<3> >& range,
	        std::vector<pConstructData >& buffers);

	//virtual bool merge(std::shared_ptr<BaseData> other);
	virtual bool merge(std::shared_ptr<ConstructData> otherConstruct);

	virtual bool merge(char* buffer, int size);

	virtual bool merge();

	virtual bool mergeStoredData();

	virtual void unserializeAndStore(char* buffer, int bufferSize);

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

	void softClean();

	std::shared_ptr<BaseConstructData> getData(std::string key);

	bool hasData(std::string key);

	bool setMergeOrder(std::vector<std::string>& merge_order); ///< applies the priority order for merge. Currently, it only checks whether the size of the merge order matches with the number of fields in the container. Checking the name of the fields is not implemented yet.
	const std::vector<std::string>& getMergeOrder(); ///< returns the priority order for merge

	bool setSplitOrder(std::vector<std::string>& split_order); ///< applies the priority order for split. Similar to merge, it doesn't yet check the name of the fields in the split order.
	const std::vector<std::string>& getSplitOrder(); ///< returns the priority order for split

	template<typename T>
	std::shared_ptr<T> getTypedData(const char* key);

	template<typename T>
	T getFieldData(const char* key);

	void updateNbItems();

	void copySystemFields(pConstructData& source); ///< copies the system fields which have to be sent to every destination regardless of the split policy

protected:
	mapConstruct container_; ///< collection of fields, the data model
	int nbFields_; ///< number of fields in the data model
	bool bZCurveKey_; ///<  true when the data field can be used to compute a Z curve (position)
	bool bZCurveIndex_; ///< true when the data field can be used to compute a Z curve (Morton code)
	std::shared_ptr<BaseConstructData> zCurveKey_; ///< the field containing the positions to compute the ZCurve (position)
	std::shared_ptr<BaseConstructData> zCurveIndex_;  ///< the field containing the positions to compute the ZCurve (Morton code)

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(container_);
		ar & BOOST_SERIALIZATION_NVP(nbItems_);
	}

	bool updateMetaData();

	std::string out_serial_buffer_; ///< output buffer for serialization
	std::string in_serial_buffer_; ///< input buffer to fetch the serialized data (before deserializing it)

	std::vector<std::string> merge_order_; ///< a priority order for merge
	std::vector<std::string> split_order_; ///< a priority order for split

	std::vector<std::shared_ptr<std::map<std::string, datafield> > > partialData;
	std::vector<std::vector<int> > rangeItems_; ///< used for computing indexes from blocks -- related to zCurve (position and Morton) flag
	bool bSystem_;
    	bool bToken_;
	int nbSystemFields_; ///< number of system fields in the data model
	bool bEmpty_; ///< if it is false: there is some user data
	bool bCountable_; ///< indicates whether the data model is countable/splitable
	bool bPartialCountable_;
};

//Have to define it here because of the template
template<typename T>
std::shared_ptr<T>
decaf::
ConstructData::getTypedData(const char* key)
{
	std::shared_ptr<BaseConstructData> field = this->getData(key);
	if(!field)
	{
		return std::shared_ptr<T>();
	}

	std::shared_ptr<T> result = std::dynamic_pointer_cast<T>(field);

	//Checking if the pointer is valid
	assert(result);

	return result;
}

template<typename T>
T
decaf::
ConstructData::getFieldData(const char* key)
{
	std::shared_ptr<BaseConstructData> field = this->getData(key);
	if(!field)
	{
		//std::cerr<<"Fail cast in getFieldData when requesting the field \""<<key<<"\""<<std::endl;
		return T();
	}

	return T(field);
}


} //namespace

#endif
