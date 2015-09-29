#ifndef BLOCK_CONSTRUCT_DATA
#define BLOCK_CONSTRUCT_DATA

#include "baseconstructdata.hpp"
#include <decaf/data_model/block.hpp>
namespace decaf{


class BlockConstructData : public BaseConstructData {
public:

    BlockConstructData(Block<3> block = Block<3>(), mapConstruct map = mapConstruct())
        : BaseConstructData(map), value_(block){}

    virtual ~BlockConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
    }

    virtual bool isBlockSplitable(){ return true; }

    virtual int getNbItems(){ return 1; }

    Block<3>& getData(){ return value_; }

    virtual std::vector< std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<BlockConstructData>(value_, map_));
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<BlockConstructData>(value_, map_));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for BlockConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<BlockConstructData>(value_));
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<BlockConstructData>(value_));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for BlockConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    //This function should never be called
    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< Block<3> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch(policy)
        {
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<BlockConstructData>(value_));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for BlockConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual bool merge( std::shared_ptr<BaseConstructData> other,
                        mapConstruct partial_map,
                        ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<BlockConstructData> other_ = std::dynamic_pointer_cast<BlockConstructData>(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                return true;
                break;
            }
            case DECAF_MERGE_FIRST_VALUE: //We don't have to do anything here
            {
                return true;
                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for block data."<<std::endl;
                return false;
                break;
            }

        }
    }

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other)
    {
        std::shared_ptr<BlockConstructData> other_ = std::dynamic_pointer_cast<BlockConstructData>(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }

protected:
    Block<3> value_;
};

}//namespace

BOOST_CLASS_EXPORT_GUID(decaf::BlockConstructData,"BlockConstructData")



#endif
