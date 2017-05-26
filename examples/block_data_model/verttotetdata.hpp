#ifndef VERT_TO_TET_DATA
#define VERT_TO_TET_DATA

#include <bredala/data_model/arrayconstructdata.hpp>
#include "arraytetsdata.hpp"
#include <numeric>
#include "tess/tet.h"


namespace decaf {

class VertToTetData : public ArrayConstructData<int>
{

public:
    VertToTetData(mapConstruct map = mapConstruct()) : ArrayConstructData<int>(map){}

    VertToTetData(int* array, int size, int element_per_items, bool owner = false, mapConstruct map = mapConstruct()) :
                        ArrayConstructData<int>(array, size, element_per_items, owner, map){}

    virtual ~VertToTetData()
    {
        if(owner_) delete[] value_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<ArrayConstructData<int> >(*this);
    }

    virtual int getNbItems()
    {
        std::map<std::string, datafield>::iterator data;
        if(!map_)
        {
            std::cerr<<"ERROR : VertToTetData doesn't have access to the map."<<std::endl;
            return 0;
        }
        else
            std::cout<<"Access to the map in VertToTet"<<std::endl;

        /*std::cout<<"Size of the map : "<<map_->size()<<std::endl;

        for(std::map<std::string, datafield>::iterator it = map_->begin(); it != map_->end(); it++)
        {
            std::cout<<"Parcours d'un champ"<<std::endl;
            std::cout<<"Nom du champ : "<<it->first<<std::endl;
            std::map<std::string, datafield>::iterator test = map_->find(it->first);
            std::cout<<"Test passer"<<std::endl;
        }*/

        data = map_->find(std::string("pos"));
        std::cout<<"Find ok"<<std::endl;
        if(data == map_->end())
        {
            std::cerr<<"ERROR : Field \"pos\" not found in the map. Required for "
                    <<"the VertToTetData type split and merge."<<std::endl;
            return 0;
        }
        else
            std::cout<<"The field pos is accessible."<<std::endl;

        return std::get<3>(data->second)->getNbItems();
    }

    virtual std::vector< std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;

        switch(policy)
        {
            //We keep only the tets for which we have the 4 particles
            case DECAF_SPLIT_DEFAULT :
            {
                for(int i = 0; i < range.size(); i++)
                {
                    //Getting the number of particules and the list of tets
                    std::map<std::string, datafield>::iterator dataPart;
                    dataPart = partial_map.at(i)->find("pos");
                    if(dataPart == partial_map.at(i)->end())
                    {
                        std::cout<<"ERROR : Unable to retrieve the field \"pos\" in the partial maps "
                                 <<"when splitting the verttotet. Make sure that \"pos\" is splitted "
                                 <<" before this field."<<std::endl;
                        result.clear();
                        return result;
                    }

                    std::map<std::string, datafield>::iterator dataTets;
                    dataTets = partial_map.at(i)->find("tets");
                    if(dataTets == partial_map.at(i)->end())
                    {
                        std::cout<<"ERROR : Unable to retrieve the field \"tets\" in the partial maps "
                                 <<"when splitting the verttotet. Make sure that \"tets\" is splitted "
                                 <<" before this field."<<std::endl;
                        result.clear();
                        return result;
                    }

                    int tableSize = std::get<3>(dataPart->second)->getNbItems();
                    int* verttotet = new int[tableSize];

                    // First we fill the table with -1 for cases where a particule doesn't have
                    // a tet after the split
                    for(int j = 0; j < tableSize; j++)
                        verttotet[j] = -1;

                    // We go through all the tets. If one of the vertex of the tet doesn't have a vert
                    // yet, we update the table
                    int nbTets = std::get<3>(dataTets->second)->getNbItems();
                    std::shared_ptr<ArrayTetsData> dataTetsArray = std::dynamic_pointer_cast<ArrayTetsData>(std::get<3>(dataTets->second));
                    tet_t *tets = dataTetsArray->getArray();

                    for(int j = 0; j < nbTets; j++)
                    {
                        for(int k = 0; k < 4; k++)
                        {
                            if(verttotet[tets[j].verts[k]] == -1)
                                verttotet[tets[j].verts[k]] = j;
                        }
                    }


                    std::shared_ptr<VertToTetData> data = std::make_shared<VertToTetData>(
                                verttotet,
                                tableSize,
                                element_per_items_,
                                true
                                );
                    result.push_back(data);
                }

                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for Verttotetdata"<<std::endl;
                break;
            }
        }
        return result;
    }

    virtual std::vector<std::shared_ptr<BaseConstructData> > split(
            const std::vector< std::vector<int> >& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {

        std::vector<std::shared_ptr<BaseConstructData> > result;

        switch(policy)
        {
            //We keep only the tets for which we have the 4 particles
            case DECAF_SPLIT_DEFAULT :
            {
                for(int i = 0; i < range.size(); i++)
                {
                    //Getting the number of particules and the list of tets
                    std::map<std::string, datafield>::iterator dataPart;
                    dataPart = partial_map.at(i)->find("pos");
                    if(dataPart == partial_map.at(i)->end())
                    {
                        std::cout<<"ERROR : Unable to retrieve the field \"pos\" in the partial maps "
                                 <<"when splitting the verttotet. Make sure that \"pos\" is splitted "
                                 <<" before this field."<<std::endl;
                        result.clear();
                        return result;
                    }

                    std::map<std::string, datafield>::iterator dataTets;
                    dataTets = partial_map.at(i)->find("tets");
                    if(dataTets == partial_map.at(i)->end())
                    {
                        std::cout<<"ERROR : Unable to retrieve the field \"tets\" in the partial maps "
                                 <<"when splitting the verttotet. Make sure that \"tets\" is splitted "
                                 <<" before this field."<<std::endl;
                        result.clear();
                        return result;
                    }

                    int tableSize = std::get<3>(dataPart->second)->getNbItems();
                    int* verttotet = new int[tableSize];

                    // First we fill the table with -1 for cases where a particule doesn't have
                    // a tet after the split
                    for(int j = 0; j < tableSize; j++)
                        verttotet[j] = -1;

                    // We go through all the tets. If one of the vertex of the tet doesn't have a vert
                    // yet, we update the table
                    int nbTets = std::get<3>(dataTets->second)->getNbItems();
                    std::shared_ptr<ArrayTetsData> dataTetsArray = std::dynamic_pointer_cast<ArrayTetsData>(std::get<3>(dataTets->second));
                    tet_t *tets = dataTetsArray->getArray();

                    for(int j = 0; j < nbTets; j++)
                    {
                        for(int k = 0; k < 4; k++)
                        {
                            if(verttotet[tets[j].verts[k]] == -1)
                                verttotet[tets[j].verts[k]] = j;
                        }
                    }


                    std::shared_ptr<VertToTetData> data = std::make_shared<VertToTetData>(
                                verttotet,
                                tableSize,
                                element_per_items_,
                                true
                                );
                    result.push_back(data);
                }

                break;
            }
            default:
            {
                std::cout<<"Policy "<<policy<<" not supported for Verttotetdata"<<std::endl;
                break;
            }
        }

        return result;

    }

    virtual bool merge(std::shared_ptr<BaseConstructData> other,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        //Hypothesis : we assume that we merge the tets before the particles

        //To merge the tet, we have to shift all the references to the particles
        //by the current number of particles and all the references of the tets

        std::shared_ptr<VertToTetData> otherArray = std::dynamic_pointer_cast<VertToTetData >(other);
        if(!otherArray)
        {
            std::cout<<"ERROR : trying to merge to objects with different types"<<std::endl;
            return false;
        }

        switch(policy)
        {
            case DECAF_MERGE_DEFAULT:
            {
                // Getting the local number of particles for the shift
                int localNbParticles;
                std::map<std::string, datafield>::iterator field = partial_map->find("num_orig_particles");
                if(field == partial_map->end())
                {
                    std::cerr<<"ERROR : unable to find the field num_particles which is required "
                            <<"to merge the field of type VertToTetData."<<std::endl;
                    return false;
                }

                std::shared_ptr<SimpleConstructData<int> > numOrigParticules =
                        std::dynamic_pointer_cast<SimpleConstructData<int> >(std::get<3>(field->second));
                if(!numOrigParticules)
                {
                    std::cerr<<"ERROR : unable to cast the field num_particles with the type "
                            <<"SimpleConstructData<int> during the merge of a type VertToTetData."<<std::endl;
                    return false;
                }
                localNbParticles = numOrigParticules->getData();

                // Updating the informations of the tets in the other
                int* other_tets = otherArray->getArray();
                int otherNbTets = otherArray->getSize(); // getNbItems is the number of semantic items -> nbParticles

                for(int i = 0; i < otherNbTets; i++)
                {
                    //Shifting the particle indexes
                    if(other_tets[i] != -1)
                        other_tets[i] = other_tets[i] + localNbParticles;
                }

                //Now we can simply concatenate the arrays
                int* new_tets = new int[size_ + otherNbTets];
                memcpy(new_tets, value_, size_ * sizeof(int));
                memcpy(new_tets + size_, other_tets, otherNbTets * sizeof(int));

                //Cleaning the older array if we have to
                if(owner_) delete value_;

                value_ = new_tets;
                owner_ = true;  //We generated this array so we are the owner
                size_ = size_ + otherNbTets;

                return true;

                break;
            }
            default:
            {
                std::cout<<"ERROR : policy "<<policy<<" not available for VertToTetData."<<std::endl;
                return false;
                break;
            }
        }

    }

};

} //namespace

BOOST_CLASS_EXPORT_GUID(decaf::VertToTetData,"VertToTetData")

#endif






typedef struct
{
    unsigned int index[4];
}  tet;

typedef struct
{
    unsigned int nbPos;
    float* pos;

    unsigned int nbTets;
    tet* tets;
} block;

