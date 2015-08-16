//---------------------------------------------------------------------------
//
// decaf datamodel interface which should be reimplemented by any data
// going through a Decaf transport method such as a redistribution component
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_BASEDATA_H
#define DECAF_BASEDATA_H

#include <vector>
#include <memory>


namespace  decaf {

  class BaseData
  {
  public:
    BaseData(std::shared_ptr<void> data = NULL) :
        data_(data), splitable_(false)/*, nbElements_(0)*/, nbItems_(0){}

    virtual ~BaseData(){}

    // Return the number of items from the data object.
    // An item is a data which can not be break down anymore
    virtual unsigned int getNbItems() const { return nbItems_; }

    // Return the number of items from the data object.
    // An item is a data which can not be break down anymore
    //virtual unsigned int getNbElements() const { return nbElements_;}

    // Return the index item from the data object
    //virtual BaseData getItem(unsigned int index) = 0;

    // Return true is some field in the data can be used to
    // compute a Z curve (should be a float* field)
    virtual bool hasZCurveKey() = 0;

    // Extract the field containing the positions to compute
    // the ZCurve.
    virtual const float* getZCurveKey(int *nbItems) = 0;

    // Return true is some field in the data can be used to
    // compute a Z curve (should be a float* field)
    virtual bool hasZCurveIndex() = 0;

    // Extract the field containing the positions to compute
    // the ZCurve.
    virtual const unsigned int* getZCurveIndex(int *nbItems) = 0;

    // Return true if the data contains more than 1 item
    virtual bool isSplitable() = 0;

    // Split the data in range.size() groups. The group i contains
    // range[i] items.
    // WARNING : The sum of range[i] should be equal to getNbItems()
    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<int>& range) = 0;


    // Split the data in range.size() groups. The group i contains the items
    // with the indexes from range[i]
    // WARNING : All the indexes of the data should be included
    virtual std::vector< std::shared_ptr<BaseData> > split(
            const std::vector<std::vector<int> >& range) = 0;

    // If the BaseData can be merged with another BaseData
    // virtual bool isMergeable() = 0;

    // Insert the data from other into the current objects
    virtual bool merge(std::shared_ptr<BaseData> other) = 0;

    // Insert the data from other in its serialize form
    virtual bool merge(char* buffer, int size) = 0;

    // Merge a data previously put in the serial buffer of the data object
    virtual bool merge() = 0;

    // Convert the Map into a MPI compatible structure
    //virtual CommDatatype getMPIDatatype() const = 0;

    //Access the data. Can be cast into the structure
    std::shared_ptr<void> getData(){ return data_;}

    // Update the datamap required to generate the datatype
    virtual bool setData(std::shared_ptr<void> data) = 0;

    //Reset the data so the data model is empty
    virtual void purgeData() = 0;

    // Function to serialize the data. The produced serialization should be
    // returned by getOutSerialBuffer()
    virtual bool serialize() = 0;

    // Function which deserialized the data. The serialized data should come
    // from getInSerialBuffer()
    virtual bool unserialize() = 0;

    //Prepare enough space in the serial buffer
    virtual void allocate_serial_buffer(int size) = 0;


    virtual char* getOutSerialBuffer(int* size) = 0;
    virtual char* getOutSerialBuffer() = 0;
    virtual int getOutSerialBufferSize() = 0;

    virtual char* getInSerialBuffer(int* size) = 0;
    virtual char* getInSerialBuffer() = 0;
    virtual int getInSerialBufferSize() = 0;


  protected:
    bool splitable_;                    // Can be split into smaller chunks
    unsigned int nbItems_;
    std::shared_ptr<void> data_;        // Pointer to the actual data

  };
}

#endif
