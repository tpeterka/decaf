//---------------------------------------------------------------------------
//
// decaf datatypes implementation in mpi transport layer
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_TRANSPORT_MPI_NEWDATA_HPP
#define DECAF_TRANSPORT_MPI_NEWDATA_HPP

#include <mpi.h>
#include <vector>
#include <memory>
#include "types.hpp"
#include "../../types.hpp"

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
    virtual bool merge(shared_ptr<BaseData> other) = 0;

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

    // Add an element to the data object. If possible it is merged with the current data
    // Otherwise it is pushed back to the segment data
    // WARNING :  this version will make a copy of the data.
    // Use allocateAndAppend for a version without copy
    //virtual void appendSerialData(std::shared_ptr<char> serialdata, int size) = 0;

    // Expand the current data buffer to host a new data element
    //virtual void* allocateAndAppend(size_t size, int nbElement) = 0;

    virtual bool serialize() = 0;

    virtual bool unserialize() = 0;

    /*std::shared_ptr<char> getSerialBuffer(int* size){ *size = size_buffer_; return buffer_;}
    std::shared_ptr<char> getSerialBuffer(){ return buffer_; }
    int getSerialBufferSize(){ return size_buffer_; }

    void setSerialBuffer(std::shared_ptr<char> buffer, int size_buffer)
    {
        buffer_ = buffer;
        size_buffer_ = size_buffer;
    }*/

    //Prepare enough space in the serial buffer
    virtual void allocate_serial_buffer(int size) = 0;

    /*virtual char* getSerialBuffer(int* size) = 0;
    virtual char* getSerialBuffer() = 0;
    virtual int getSerialBufferSize() = 0;*/

    virtual char* getOutSerialBuffer(int* size) = 0;
    virtual char* getOutSerialBuffer() = 0;
    virtual int getOutSerialBufferSize() = 0;

    virtual char* getInSerialBuffer(int* size) = 0;
    virtual char* getInSerialBuffer() = 0;
    virtual int getInSerialBufferSize() = 0;

    //std::vector<char>& getSerialBuffer(int* size){ *size = size_buffer_; return buffer_; }
    //std::vector<char>& getSerialBuffer(){ return buffer_; }


    /*void setSerialBuffer(std::vector<char> buffer, int size_buffer)
    {
        buffer_ = buffer;           // We want a copy so we make sure we can manage it without
        size_buffer_ = size_buffer; // side effects
    }*/


  protected:
    //virtual void updateDataMap() = 0;

    // Description of the structure. Can be shared with a layout
    //DataMap datamap_;
    bool splitable_;                    // Can be split into smaller chunks
    //bool mergeable_;                  // Can integrate data from another object
    //unsigned int nbElements_;         // Number of elements in the data segment
    unsigned int nbItems_;
    std::shared_ptr<void> data_;        // Pointer to the actual data
    //std::shared_ptr<char> buffer_;    // Buffer use for the serialization
    //std::vector<char> buffer_;          // Buffer use for the serialization
    //int size_buffer_;
  };

  class DataLayout
  {
  public:
      DataLayout(){}
      DataLayout(BaseData* data){}
      virtual ~DataLayout(){}

      // Return the number of BaseData objects stored in the layout
      virtual unsigned int getNbElement() = 0;

      // Return the number of items (ie smallest piece of data) stored
      // in the layout
      virtual unsigned int getNbItems() = 0;

      // Add a BaseData into the layout
      virtual bool insertElement(BaseData* object) = 0;

      //Return reference on the desired element if available
      virtual BaseData* getElement(unsigned int index) = 0;

      //Clear the container
      virtual void clear() = 0;

      // Build the MPI datatype for the layout
      //virtual CommDatatype getMPIDatatype() =  0;

  };


  class DataLayoutArray : public DataLayout
  {
  public:
      DataLayoutArray(BaseData* data)
      {
          datalayout_.push_back(data);
      }

      virtual ~DataLayoutArray()
      {
          datalayout_.clear();
      }

      virtual unsigned int getNbElement()
      {
          return datalayout_.size();
      }

      virtual unsigned int getNbItems()
      {
          unsigned int count = 0;
          for(unsigned int i = 0; i < datalayout_.size(); i++)
              count += datalayout_.at(i)->getNbItems();
          return count;
      }

      virtual bool insertElement(BaseData* object)
      {
          datalayout_.push_back(object);
          return true;
      }

      virtual BaseData* getElement(unsigned int index)
      {
          if(datalayout_.size() > index)
              return datalayout_.at(index);
          else
              return NULL;
      }

      virtual void clear()
      {
          datalayout_.clear();
      }

      /*virtual CommDatatype getMPIDatatype()
      {
            if(datalayout_.empty())
                return 0;

            return datalayout_.at(0)->getMPIDatatype();

      }*/

  protected:
      std::vector<BaseData*> datalayout_;
  };
}

#endif
