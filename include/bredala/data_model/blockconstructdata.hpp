#ifndef BLOCK_CONSTRUCT_DATA
#define BLOCK_CONSTRUCT_DATA

#include "baseconstructdata.hpp"
#include <bredala/data_model/block.hpp>
namespace decaf{


class BlockConstructData : public BaseConstructData {
public:

    BlockConstructData(Block<3> block = Block<3>(), mapConstruct map = mapConstruct())
        : BaseConstructData(map, false), value_(block){}

    virtual ~BlockConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
    }

    virtual bool isBlockSplitable(){ return true; }

    virtual int getNbItems(){ return 1; }

	virtual std::string getTypename(){ return std::string("Block"); } //TODO VERIFY !!

    Block<3>& getData(){ return value_; }

    Block<3>* getBlock(){ return &value_; }

    virtual bool appendItem(std::shared_ptr<BaseConstructData> dest, unsigned int index, ConstructTypeMergePolicy = DECAF_MERGE_DEFAULT)
    {
	return true;
    }

    virtual void preallocMultiple(int nbCopies , int nbItems, std::vector<std::shared_ptr<BaseConstructData> >& result)
    {
	for(unsigned int i = 0; i < nbCopies; i++)
        {
                result.push_back(std::make_shared<BlockConstructData>());
        }	
    }


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

    virtual void split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<BlockConstructData> block = std::dynamic_pointer_cast<BlockConstructData>(fields[i]);
                    assert(block);
                    block->value_ = value_;
                    block->map_ = map_;
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<BlockConstructData> block = std::dynamic_pointer_cast<BlockConstructData>(fields[i]);
                    assert(block);
                    block->value_ = value_;
                    block->map_ = map_;
                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for BlockConstructData"<<std::endl;
                break;
            }
        }
        return;
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

    virtual void split(
            const std::vector< std::vector<int> >& range,
	    std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
	return;
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

        Block<3>* otherBlock = other_->getBlock();

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                value_.makeUnion(*otherBlock);
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

    virtual bool merge(std::vector<std::shared_ptr<BaseConstructData> >& others,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
	switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                fprintf(stderr,"To be implemented\n");
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

    // TODO : reset the block values?
    virtual void softClean()
    {
	return; // Nothing to do there
    }
protected:
    Block<3> value_;
};

}//namespace

#endif
