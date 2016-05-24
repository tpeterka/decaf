#ifndef BASEFIELD_HPP
#define BASEFIELD_HPP

#include <decaf/data_model/baseconstructdata.hpp>
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

    virtual std::shared_ptr<BaseConstructData> getBasePtr() = 0;

    virtual bool empty() = 0;

};

}


#endif
