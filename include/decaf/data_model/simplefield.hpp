#ifndef SIMPLEFIELD_HPP
#define SIMPLEFIELD_HPP

#include <decaf/data_model/basefield.hpp>
#include <decaf/data_model/simpleconstructdata.hpp>
#include <memory>


namespace decaf {


template <typename T>
class SimpleField : public BaseField {

public:
    SimpleField(std::shared_ptr<BaseConstructData> ptr)
    {
        ptr_ = std::dynamic_pointer_cast<SimpleConstructData<T> >(ptr);
        if(!ptr_)
            std::cerr<<"ERROR : Unable to cast pointer to SimpleConstructData<T> when using a SimpleField."<<std::endl;
    }

   SimpleField(mapConstruct map = mapConstruct())
   {
       ptr_ = std::make_shared<SimpleConstructData<T> >(map);
   }

   SimpleField(T value, mapConstruct map = mapConstruct())
   {
       ptr_ = std::make_shared<SimpleConstructData<T> >(value, map);
   }

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

   int getNbItems()
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
typedef SimpleField<double> SimpleFliedd;

} // namespace
#endif
