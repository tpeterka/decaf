#ifndef VECTORFIELD_HPP
#define VECTORFIELD_HPP

#include <bredala/data_model/basefield.hpp>
#include <bredala/data_model/vectorconstructdata.hpp>
#include <memory>


namespace decaf {

//! Field with a vector type.
template <typename T>
class VectorField : public BaseField {

public:
    VectorField(std::shared_ptr<BaseConstructData> ptr)
    {
        ptr_ = std::dynamic_pointer_cast<VectorConstructData<T> >(ptr);
        if(!ptr_)
            std::cerr<<"ERROR : Unable to cast pointer to VectorConstructData<T> when using a VectorField."<<std::endl;
    }

    VectorField(bool init = false,
                mapConstruct map = mapConstruct(),
                bool bCountable = true)
    {
        if(init)
            ptr_ = std::make_shared<VectorConstructData<T> >(map, bCountable);
    }

    /// @param value:       		the vector values
    /// @param element_per_items:       number of required items to construct a semantic item
    /// @param map:			a container, map of fields with the field name as key and with additional information per field
    /// @param bCountable:   		indicates whether this data type is countable or not

    VectorField(std::vector<T>& value, int element_per_items, mapConstruct map = mapConstruct(),
                bool bCountable = true)
    {
        ptr_ = std::make_shared<VectorConstructData<T> >(value, element_per_items, map, bCountable);
    }

    VectorField(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end,
                int element_per_items, mapConstruct map = mapConstruct(), bool bCountable = true)
    {
        ptr_ = std::make_shared<VectorConstructData<T> >(begin, end, element_per_items, map, bCountable);
    }

    VectorField(T* Vector, int size, int element_per_items,
                mapConstruct map = mapConstruct(), bool bCountable = true)
    {
        ptr_ = std::make_shared<VectorConstructData<T> >(Vector, size, element_per_items, map, bCountable);
    }
    VectorField(int element_per_items, int capacity, mapConstruct map = mapConstruct(), bool bCountable = true)
    {
        ptr_ = std::make_shared<VectorConstructData<T> >(element_per_items, capacity, map, bCountable);
    }

    virtual ~VectorField(){}

    virtual BaseConstructData* operator -> () const
    {
        return ptr_.get();
    }

    virtual std::shared_ptr<BaseConstructData> getBasePtr()
    {
        return ptr_;
    }

    std::shared_ptr<VectorConstructData<T> > getPtr()
    {
        return ptr_;
    }

    bool empty()
    {
        return ptr_.use_count() == 0;
    }

    std::vector<T>& getVector()
    {
        return ptr_->getVector();
    }

    virtual int getNbItems()
    {
        return ptr_->getNbItems();
    }

    virtual operator bool() const {
          return ptr_ ? true : false;
        }

private:
    std::shared_ptr<VectorConstructData<T> > ptr_;
};

typedef VectorField<int> VectorFieldi;
typedef VectorField<unsigned int> VectorFieldu;
typedef VectorField<float> VectorFieldf;
typedef VectorField<double> VectorFliedd;

} // namespace
#endif
