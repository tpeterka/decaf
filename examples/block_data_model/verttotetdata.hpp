#ifndef VERT_TO_TET_DATA
#define VERT_TO_TET_DATA

#include "arrayconstructdata.hpp"

namespace decaf {

class ArrayVertToTetData : public ArrayConstructData<int>
{

public:
    ArrayVertToTetData(mapConstruct map = mapConstruct()) : ArrayConstructData<tet_t>(map){}

    ArrayVertToTetData(tet_t* array, int size, int element_per_items, bool owner = false, mapConstruct map = mapConstruct()) :
                        ArrayConstructData<tet_t>(array, size, element_per_items, owner, map){}

    virtual ~ArrayVertToTetData()
    {
        if(owner_) delete[] value_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<ArrayConstructData<tet_t> >(*this);
    }

    virtual std::vector< std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        // Here we just recompute the array based on the tets and particles we have
        // already splitted

        std::vector<std::shared_ptr<BaseConstructData> > result;

        std::map<std::string, datafield>::iterator field = partial_map->find("pos");
        if(field == partial_map->end())
        {
            std::cerr<<"ERROR : unable to find the field num_particles which is required "
                    <<"to merge the field of type ArrayTetsData."<<std::endl;
            return result;
        }
        int nbParticles = std::get<3>(field->second)->getNbItems()



    }

};


#endif
