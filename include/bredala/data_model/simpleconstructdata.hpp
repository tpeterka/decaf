#ifndef SIMPLE_CONSTRUCT_DATA
#define SIMPLE_CONSTRUCT_DATA

#include "baseconstructdata.hpp"
#include <bredala/data_model/block.hpp>
namespace decaf{


template<typename T>
class SimpleConstructData : public BaseConstructData {
public:

    SimpleConstructData(mapConstruct map = mapConstruct(), bool bCountable = true)
        : BaseConstructData(map, bCountable){}

    SimpleConstructData(const T& value, mapConstruct map = mapConstruct(), bool bCountable = true)
        : value_(value), BaseConstructData(map, bCountable){}

    SimpleConstructData(T* value, mapConstruct map = mapConstruct(), bool bCountable = true)
        : value_(*value), BaseConstructData(map, bCountable){}

    virtual ~SimpleConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
    }

    virtual bool isBlockSplitable(){ return false; }

    virtual int getNbItems(){ return 1; }

	virtual std::string getTypename(){ return boost::typeindex::type_id<T>().pretty_name(); }

    T& getData(){ return value_; }

    void setData( T& value){ value_ = value; }

    virtual bool appendItem(std::shared_ptr<BaseConstructData> dest, unsigned int index, ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
	std::cout<<"AppendItem not implemented yet for simpleConstructData."<<std::endl;
	return false;
    }

    virtual void preallocMultiple(int nbCopies , int nbItems, std::vector<std::shared_ptr<BaseConstructData> >& result)
    {
	for(unsigned int i = 0; i < nbCopies; i++)
	{
		result.push_back(std::make_shared<SimpleConstructData>());
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
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_, map_));
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_, map_));
                break;
            }
            case DECAF_SPLIT_MINUS_NBITEM:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(range.at(i), map_));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for SimpleConstructData"<<std::endl;
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
        assert(fields.size() == range.size());

        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<SimpleConstructData<T> > ptr =
                            std::dynamic_pointer_cast<SimpleConstructData<T> >(fields[i]);

                    ptr->value_ = value_;
                    ptr->map_ = map_;
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<SimpleConstructData<T> > ptr =
                            std::dynamic_pointer_cast<SimpleConstructData<T> >(fields[i]);

                    ptr->value_ = value_;
                    ptr->map_ = map_;
                }
                break;
            }
		     case DECAF_SPLIT_MINUS_NBITEM:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<SimpleConstructData<T> > ptr =
                            std::dynamic_pointer_cast<SimpleConstructData<T> >(fields[i]);

                    ptr->value_ = range[i];
                    ptr->map_ = map_;
                }
                break;
		    }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for SimpleConstructData"<<std::endl;
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
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_));
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(value_));
                break;
            }
            case DECAF_SPLIT_MINUS_NBITEM:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                    result.push_back(std::make_shared<SimpleConstructData<T> >(range.at(i).size()));
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for SimpleConstructData"<<std::endl;
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
        fprintf(stderr,"Split by block not implemented with SimpleField\n.");
        std::vector<std::shared_ptr<BaseConstructData> > result;
        return result;
    }

    virtual void split(
	    const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            std::vector<std::shared_ptr<BaseConstructData> >& fields,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        switch(policy)
        {
            case DECAF_SPLIT_DEFAULT :
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<SimpleConstructData<T> > basefield_it =
                            std::dynamic_pointer_cast<SimpleConstructData<T> >(fields[i]);
                    if(!basefield_it) fprintf(stderr, "Fail to cast in SimpleConstructData during split\n");
                    basefield_it->value_ = this->value_;
                }
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<SimpleConstructData<T> > basefield_it =
                            std::dynamic_pointer_cast<SimpleConstructData<T> >(fields[i]);
                    if(!basefield_it) fprintf(stderr, "Fail to cast in SimpleConstructData during split\n");
                    basefield_it->value_ = this->value_;
                }
                break;
            }
            case DECAF_SPLIT_MINUS_NBITEM:
            {
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<SimpleConstructData<T> > basefield_it =
                            std::dynamic_pointer_cast<SimpleConstructData<T> >(fields[i]);
                    if(!basefield_it) fprintf(stderr, "Fail to cast in SimpleConstructData during split\n");
                    basefield_it->value_ = range.at(i).size();
                }
                break;
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for SimpleConstructData"<<std::endl;
                break;
            }
        }
	return;
    }

    virtual bool merge( std::shared_ptr<BaseConstructData> other,
                        mapConstruct partial_map,
                        ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<SimpleConstructData<T> > other_ = std::dynamic_pointer_cast<SimpleConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT: //We just keep the first value
            {

                return true;
                break;
            }
            case DECAF_MERGE_FIRST_VALUE: //We don't have to do anything here
            {
                return true;
                break;
            }
            case DECAF_MERGE_ADD_VALUE:
            {
                value_ = value_ + other_->value_;
                return true;
                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for simple data."<<std::endl;
                return false;
                break;
            }

        }
    }

    virtual bool merge(std::vector<std::shared_ptr<BaseConstructData> >& others,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        for(unsigned int i = 0; i < others.size(); i++)
        {
            std::shared_ptr<SimpleConstructData<T> > other_ = std::dynamic_pointer_cast<SimpleConstructData<T> >(others[i]);
            switch(policy)
            {
                case DECAF_MERGE_DEFAULT: //We just keep the first value
                {
                    break;
                }
                case DECAF_MERGE_FIRST_VALUE: //We don't have to do anything here
                {

                    break;
                }
                case DECAF_MERGE_ADD_VALUE:
                {
                    value_ = value_ + other_->value_;

                    break;
                }
                default:
                {
                    std::cout<<"ERROR : policy "<<policy<<" not available for simple data."<<std::endl;
                    return false;
                    break;
                }
            }
        }
        return true;
    }

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other)
    {
        std::shared_ptr<SimpleConstructData<T> > other_ = std::dynamic_pointer_cast<SimpleConstructData<T> >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }

    virtual void softClean()
    {
	return; //Nothing to do here
    }
protected:
    T value_;
};

}//namespace

#endif
