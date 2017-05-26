#ifndef TET_COUNTER_DATA
#define TET_COUNTER_DATA

#include <bredala/data_model/simpleconstructdata.hpp>

namespace decaf{

class TetCounterData : public SimpleConstructData<int> {
  public:

    TetCounterData(mapConstruct map = mapConstruct())
        : SimpleConstructData<int>(map){}

    TetCounterData(int value, mapConstruct map = mapConstruct())
        : SimpleConstructData<int>(value, map){}

    virtual ~TetCounterData(){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<SimpleConstructData<int> >(*this);
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
                {
                    std::map<std::string, datafield>::iterator data;
                    data = partial_map.at(i)->find("tets");
                    if(data == map_->end())
                    {
                        std::cerr<<"ERROR : Field \"tets\" not found in the map. Required for "
                                <<"the TetCounterData type split and merge."<<std::endl;
                        result.push_back(std::make_shared<TetCounterData >(0, map_));
                    }
                    int nbItems =  std::get<3>(data->second)->getNbItems();
                    result.push_back(std::make_shared<TetCounterData >(nbItems, map_));
                }
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
                {
                    std::map<std::string, datafield>::iterator data;
                    data = partial_map.at(i)->find("tets");
                    if(data == map_->end())
                    {
                        std::cerr<<"ERROR : Field \"tets\" not found in the map. Required for "
                                <<"the TetCounterData type split and merge."<<std::endl;
                        result.push_back(std::make_shared<TetCounterData >(0, map_));
                    }
                    int nbItems =  std::get<3>(data->second)->getNbItems();
                    result.push_back(std::make_shared<TetCounterData >(nbItems, map_));
                }
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
};

} //namespace

BOOST_CLASS_EXPORT_GUID(decaf::TetCounterData,"TetCounterData")


#endif
