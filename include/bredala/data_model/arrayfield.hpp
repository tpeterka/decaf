#ifndef ARRAYFIELD_HPP
#define ARRAYFIELD_HPP

#include <bredala/data_model/basefield.hpp>
#include <bredala/data_model/arrayconstructdata.hpp>
#include <memory>


namespace decaf {

//! Field with an array type.
template <typename T>
class ArrayField : public BaseField {

public:
    ArrayField(std::shared_ptr<BaseConstructData> ptr)
    {
        ptr_ = std::dynamic_pointer_cast<ArrayConstructData<T> >(ptr);
        if(!ptr_)
        {
            std::cerr<<"ERROR : Unable to cast pointer to ArrayConstructData<T> when using a ArrayField."<<std::endl;
            ptr_.reset();
        }
    }

    ArrayField(bool init = false, mapConstruct map = mapConstruct())
    {
        if (init)
            ptr_ = std::make_shared<ArrayConstructData<T> >(map);
    }

    /// @param array:                the array
    /// @param size:                 size of the array
    /// @param element_per_items:    number of required items to construct a semantic item
    /// @param map:		     a container, map of fields with the field name as key and with additional information per field
    /// @param bCountable:   	     indicates whether this data type is countable or not

    ArrayField(T* array,
               int size,
               int element_per_items,
               bool owner = false,
               mapConstruct map = mapConstruct(),
               bool bCountable = true)
    {
        ptr_ = std::make_shared<ArrayConstructData<T> >
                (array, size, element_per_items, owner, map, bCountable);
    }

    ArrayField(T* array,
               int size,
               int element_per_items,
               int capacity,
               bool owner = false,
               mapConstruct map = mapConstruct(),
               bool bCountable = true)
    {
        ptr_ = std::make_shared<ArrayConstructData<T> >
                ( array, size, element_per_items, capacity, owner, map, bCountable);
    }

    ArrayField(std::vector<std::pair<T*, unsigned int> > segments,
               int element_per_items,
               mapConstruct map = mapConstruct(),
               bool bCountable = true)
    {
        ptr_ = std::make_shared<ArrayConstructData<T> >(segments, element_per_items, map, bCountable);
    }

    virtual ~ArrayField(){}

    virtual BaseConstructData* operator -> () const
    {
        return ptr_.get();
    }

    virtual std::shared_ptr<BaseConstructData> getBasePtr()
    {
        return ptr_;
    }

    std::shared_ptr<ArrayConstructData<T> > getPtr()
    {
        return ptr_;
    }

    bool empty()
    {
        return ptr_.use_count() == 0;
    }

    void reset()
    {
        ptr_.reset();
    }

    T* getArray()
    {
        return ptr_->getArray();
    }

    int getArraySize()
    {
        return ptr_->getSize();
    }

    virtual int getNbItems()
    {
        return ptr_->getNbItems();
    }

    virtual operator bool() const
    {
        return ptr_ ? true : false;
    }



private:
    std::shared_ptr<ArrayConstructData<T> > ptr_;
};

typedef ArrayField<int> ArrayFieldi;
typedef ArrayField<unsigned int> ArrayFieldu;
typedef ArrayField<float> ArrayFieldf;
typedef ArrayField<double> ArrayFieldd;
typedef ArrayField<char> ArrayFieldc;

} // namespace
#endif
