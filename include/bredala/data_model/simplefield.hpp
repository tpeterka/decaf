#ifndef SIMPLEFIELD_HPP
#define SIMPLEFIELD_HPP

#include <bredala/data_model/basefield.hpp>
#include <bredala/data_model/simpleconstructdata.hpp>
#include <memory>


namespace decaf {

//! Field with a simple data type.
template <typename T>
class SimpleField : public BaseField {

public:
    SimpleField(std::shared_ptr<BaseConstructData> ptr)
    {
        ptr_ = std::dynamic_pointer_cast<SimpleConstructData<T> >(ptr);
        if(!ptr_)
            std::cerr<<"ERROR : Unable to cast pointer to SimpleConstructData<T> when using a SimpleField."<<std::endl;
    }

   SimpleField(bool init = false,
               mapConstruct map = mapConstruct(),
               bool bCountable = true)
   {
        if(init)
            ptr_ = std::make_shared<SimpleConstructData<T> >(map, bCountable);
   }

   /// @param value:       		the data point
   /// @param map:			a container, map of fields with the field name as key and with additional information per field
   /// @param bCountable:   		indicates whether this data type is countable or not

   SimpleField(const T& value, mapConstruct map = mapConstruct(),
               bool bCountable = true)
   {
       ptr_ = std::make_shared<SimpleConstructData<T> >(value, map, bCountable);
   }

   SimpleField(T* value, mapConstruct map = mapConstruct(),
               bool bCountable = true)
   {
       ptr_ = std::make_shared<SimpleConstructData<T> >(value, map, bCountable);
   }

   virtual ~SimpleField(){}

   virtual BaseConstructData* operator -> () const
   {
       return ptr_.get();
   }

   virtual std::shared_ptr<BaseConstructData> getBasePtr()
   {
       return ptr_;
   }

   std::shared_ptr<SimpleConstructData<T> > getPtr()
   {
       return ptr_;
   }

   bool empty()
   {
       return ptr_.use_count() == 0;
   }

   T& getData()
   {
       return ptr_->getData();
   }

   virtual int getNbItems()
   {
       return ptr_->getNbItems();
   }

   virtual operator bool() const {
         return ptr_ ? true : false;
       }


private:
    std::shared_ptr<SimpleConstructData<T> > ptr_;
};

typedef SimpleField<int> SimpleFieldi;
typedef SimpleField<unsigned int> SimpleFieldu;
typedef SimpleField<float> SimpleFieldf;
typedef SimpleField<double> SimpleFieldd;

} // namespace
#endif
