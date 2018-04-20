#ifndef BLOCKFIELD
#define BLOCKFIELD

#include <bredala/data_model/basefield.hpp>
#include <bredala/data_model/blockconstructdata.hpp>
#include <memory>

namespace decaf {
//! Field with a block data type.
class BlockField : public BaseField {

public:
    BlockField(std::shared_ptr<BaseConstructData> ptr)
    {
        ptr_ = std::dynamic_pointer_cast<BlockConstructData>(ptr);
        if(!ptr_)
        {
            std::cerr<<"ERROR : Unable to cast pointer to BlockConstructData when using a ArrayField."<<std::endl;
            ptr_.reset();
        }
    }

    BlockField(bool init = false, mapConstruct map = mapConstruct())
    {
        if(init)
            ptr_ = std::make_shared<BlockConstructData>(Block<3>(), map);
    }

    BlockField(Block<3>& block, mapConstruct map = mapConstruct())
    {
        ptr_ = std::make_shared<BlockConstructData>(block, map);
    }

    virtual ~BlockField(){}

    virtual BaseConstructData* operator -> () const
    {
        return ptr_.get();
    }

    virtual std::shared_ptr<BaseConstructData> getBasePtr()
    {
        return ptr_;
    }

    std::shared_ptr<BlockConstructData> getPtr()
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

    Block<3>* getBlock()
    {
        return ptr_->getBlock();
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
    std::shared_ptr<BlockConstructData> ptr_;

};

}

#endif
