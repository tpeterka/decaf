#ifndef BBOX_CONSTRUCT_DATA
#define BBOX_CONSTRUCT_DATA

#include <decaf/data_model/vectorconstructdata.hpp>
#include "arrayconstructdata.hpp"

namespace decaf {


class BBoxConstructData : public VectorConstructData<float> {
public:
    BBoxConstructData(mapConstruct map = mapConstruct())
        : VectorConstructData<float>(map){}

    BBoxConstructData(std::vector<float>& value, mapConstruct map = mapConstruct()) :
                       VectorConstructData<float>(value, 6, map)
    {
        if(value.size() != 6)
        {
            std::cerr<<"ERROR : bbox given has not a size 6. Abording."<<std::endl;
            exit(1);
        }
    }

    BBoxConstructData(std::vector<float>::iterator begin, std::vector<float>::iterator end)
        : VectorConstructData<float>()
    {
        if(end - begin != 6)
        {
            std::cerr<<"ERROR : bbox given has not a size 6. Abording."<<std::endl;
            exit(1);
        }
        element_per_items_ = 6;
        value_ = std::vector<float>(begin, end);
    }
    BBoxConstructData(float* Vector, int size) : VectorConstructData<float>()
    {
        if(size != 6)
        {
            std::cerr<<"ERROR : bbox given has not a size 6. Abording."<<std::endl;
            exit(1);
        }
        element_per_items_ = 6;
        value_ = std::vector<float>(Vector, Vector + size);
    }

    BBoxConstructData(float* min, float* max)
    {
        value_.resize(6);
        for(unsigned int i = 0; i < 3; i++)
            value_[i] = min[i];
        for(unsigned int i = 0; i < 3; i++)
            value_[i+3] = max[i];
        element_per_items_ = 6;

    }

    virtual ~BBoxConstructData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<BaseConstructData>(*this);
        ar & BOOST_SERIALIZATION_NVP(value_);
        ar & BOOST_SERIALIZATION_NVP(element_per_items_);
    }

   virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT )
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                std::shared_ptr<BBoxConstructData> sub = std::make_shared<BBoxConstructData>(
                            value_.begin(), value_.end());
                result.push_back(sub);
                break;
            }
            case DECAF_SPLIT_KEEP_VALUE:
            {
                std::shared_ptr<BBoxConstructData> sub = std::make_shared<BBoxConstructData>(
                            value_.begin(), value_.end());
                result.push_back(sub);
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for BBoxConstructData"<<std::endl;
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
        switch( policy )
        {
            case DECAF_SPLIT_DEFAULT:
            {
                //Sanity check
                int totalRange = 0;
                for(unsigned int i = 0; i < range.size(); i++)
                    totalRange+= range.at(i).size();
                if(totalRange != getNbItems()){
                    std::cout<<"ERROR : The number of items in the ranges ("<<totalRange
                             <<") does not match the number of items of the object ("
                             <<getNbItems()<<")"<<std::endl;
                    return result;
                }

                typename std::vector<float>::iterator it = value_.begin();
                for(unsigned int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<BBoxConstructData> sub = std::make_shared<BBoxConstructData>(
                                value_.begin(), value_.end());
                    result.push_back(sub);
                }
                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for BBoxConstructData"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual bool merge( std::shared_ptr<BaseConstructData> other,
                        mapConstruct partial_map,
                        ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        std::shared_ptr<BBoxConstructData > other_ = dynamic_pointer_cast<BBoxConstructData >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                if(value_ != other_->value_)
                {
                    for(unsigned int i = 0; i < 3; i++)
                        value_[i] = min(value_[i], other_->value_[i]);
                    for(unsigned int i = 3; i < 6; i++)
                        value_[i] = max(value_[i], other_->value_[i]);
                }
                return true;
                break;
            }
            case DECAF_MERGE_BBOX_POS:
            {
                std::map<std::string, datafield>::iterator field = partial_map->find("pos");
                if(field == partial_map->end())
                {
                    std::cerr<<"ERROR : The field pos does not exist in the partial map. "
                            <<"Unable to compute the bbox from the positions."<<std::endl;
                }
                //We try the possible field conversion
                bool success_conv = false;
                std::shared_ptr<VectorConstructData<float> > posvf =
                        dynamic_pointer_cast<VectorConstructData<float> >(posvf);
                if(posvf)
                {
                    success_conv = true;
                    std::vector<float>& pos = posvf->getVector();
                    computeBBoxFromPosF(&pos[0], posvf->getNbItems());
                }
                std::shared_ptr<VectorConstructData<double> > posvd =
                        dynamic_pointer_cast<VectorConstructData<double> >(posvd);
                if(posvd)
                {
                    success_conv = true;
                    std::vector<double>& pos = posvd->getVector();
                    computeBBoxFromPosD(&pos[0], posvd->getNbItems());
                }
                std::shared_ptr<ArrayConstructData<float> > posaf =
                        dynamic_pointer_cast<ArrayConstructData<float> >(posaf);
                if(posaf)
                {
                    success_conv = true;
                    float* pos = posaf->getArray();
                    computeBBoxFromPosF(pos, posaf->getNbItems());
                }
                std::shared_ptr<ArrayConstructData<double> > posad =
                        dynamic_pointer_cast<ArrayConstructData<double> >(posad);
                if(posad)
                {
                    success_conv = true;
                    double* pos = posad->getArray();
                    computeBBoxFromPosD(pos, posad->getNbItems());
                }

                if(!success_conv)
                    std::cerr<<"ERROR : Unable to convert properly the field pos to compute the bbox."<<std::endl;
                return success_conv;

            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for simple data."<<std::endl;
                return false;
                break;
            }
        }
    }

    virtual bool canMerge(std::shared_ptr<BaseConstructData> other)
    {
        std::shared_ptr<BBoxConstructData>other_ = dynamic_pointer_cast<BBoxConstructData >(other);
        if(!other_)
        {
            std::cout<<"ERROR : trying to merge two objects with different types"<<std::endl;
            return false;
        }
        return true;
    }

protected:

    void computeBBoxFromPosF(float* pos, int nbPos)
    {
        for(int i = 0; i < nbPos; i++)
        {
            for(unsigned int j = 0; j < 3; j++)
                value_[j] = min(value_[j], pos[3*i+j]);
            for(unsigned int j = 3; j < 6; j++)
                value_[j] = max(value_[j], pos[3*i+j]);
        }
    }

    void computeBBoxFromPosD(double* pos, int nbPos)
    {
        for(int i = 0; i < nbPos; i++)
        {
            for(unsigned int j = 0; j < 3; j++)
                value_[j] = min(value_[j], (float)(pos[3*i+j]));
            for(unsigned int j = 3; j < 6; j++)
                value_[j] = max(value_[j], (float)(pos[3*i+j]));
        }
    }
};

} //namespace

BOOST_CLASS_EXPORT_GUID(decaf::BBoxConstructData,"BBoxConstructData")

#endif
