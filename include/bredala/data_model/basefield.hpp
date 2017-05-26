#ifndef BASEFIELD_HPP
#define BASEFIELD_HPP

#include <bredala/data_model/baseconstructdata.hpp>
#include <memory>

namespace decaf {

class BaseField {

public:
    BaseField(){}
    BaseField(std::shared_ptr<BaseConstructData> ptr){}

    virtual BaseConstructData* operator -> () const = 0;

    virtual operator bool() const {
        return false;
      }

    virtual ~BaseField(){}

    virtual std::shared_ptr<BaseConstructData> getBasePtr() = 0;

    virtual bool empty() = 0;

    virtual int getNbItems() = 0;

};

}


#endif
