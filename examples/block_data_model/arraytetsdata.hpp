#ifndef ARRAY_TETS_DATA
#define ARRAY_TETS_DATA

#include <bredala/data_model/arrayconstructdata.hpp>
#include <numeric>
#include "tess/tet.h"

//Code to serialize the tet_t structure
namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, tet_t & g, const unsigned int version)
{
    ar & g.tets;
    ar & g.verts;
}

} // namespace serialization
} // namespace boost

namespace decaf {

class ArrayTetsData : public ArrayConstructData<tet_t>
{

public:
    ArrayTetsData(mapConstruct map = mapConstruct()) : ArrayConstructData<tet_t>(map){}

    ArrayTetsData(tet_t* array, int size, int element_per_items, bool owner = false, mapConstruct map = mapConstruct()) :
                        ArrayConstructData<tet_t>(array, size, element_per_items, owner, map){}

    virtual ~ArrayTetsData()
    {
        if(owner_) delete[] value_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        boost::serialization::base_object<ArrayConstructData<tet_t> >(*this);
    }

    virtual int getNbItems()
    {
        std::map<std::string, datafield>::iterator data;
        data = map_->find(std::string("pos"));
        if(data == map_->end())
        {
            std::cerr<<"ERROR : Field \"pos\" not found in the map. Required for "
                    <<"the ArrayTetsData type split and merge."<<std::endl;
            return 0;
        }

        return std::get<3>(data->second)->getNbItems();
    }

    virtual std::vector< std::shared_ptr<BaseConstructData> > split(
            const std::vector<int>& range,
            std::vector< mapConstruct >& partial_map,
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {
        std::vector<std::shared_ptr<BaseConstructData> > result;
        // Index of the split for each tet. -1 is no split
        std::vector<int> assigned_split(size_);
        // Number of tets for each split
        std::vector<int> range_sum(range.size(),0);

        std::vector<int> ranges;
        std::partial_sum(range.begin(), range.end(), ranges.begin());

        switch(policy)
        {
            //We keep only the tets for which we have the 4 particles
            case DECAF_SPLIT_DEFAULT :
            {
                for(int i = 0; i < size_; i++)
                {
                    //Computing the split for the first particle
                    int split_first_particle;
                    for(unsigned int j = 0; j <  ranges.size(); i++)
                    {
                        if(value_[i].verts[0] < ranges.at(j))
                        {
                            split_first_particle = j;
                            break;
                        }
                    }

                    //Checking if the other particles are in the same split
                    bool part2,part3,part4;
                    if(split_first_particle == 0)
                    {
                        part2 = value_[i].verts[1] < ranges.at(split_first_particle);
                        part3 = value_[i].verts[2] < ranges.at(split_first_particle);
                        part4 = value_[i].verts[3] < ranges.at(split_first_particle);
                    }
                    else
                    {
                        part2 = value_[i].verts[1] < ranges.at(split_first_particle)
                                && value_[i].verts[1]>= ranges.at(split_first_particle-1);
                        part3 = value_[i].verts[2] < ranges.at(split_first_particle)
                                && value_[i].verts[2]>= ranges.at(split_first_particle-1);
                        part4 = value_[i].verts[3] < ranges.at(split_first_particle)
                                && value_[i].verts[3]>= ranges.at(split_first_particle-1);
                    }

                    //If all particles are in the same split, we can take this tet
                    if(part2 && part3 && part4)
                    {
                        assigned_split[i] = split_first_particle;
                        range_sum[split_first_particle] += 1;
                    }
                    else
                        assigned_split[i] = -1;

                }

                //Now we can generate the arrays for the split
                std::vector<int> current_indexes(range.size(),0); //To keep track where to write the tets
                std::vector<tet_t*> arrays_tet;

                //Initializing the arrays
                for(int i = 0; i < range.size(); i++)
                    arrays_tet.push_back(new tet_t[range_sum.at(i)]);

                //Writting the tets
                for(int i = 0; i < size_; i++)
                {
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[0] = value_[i].verts[0];
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[1] = value_[i].verts[1];
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[2] = value_[i].verts[2];
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[3] = value_[i].verts[3];

                    //For now we don't keep the neighors
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[0] = -1;
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[1] = -1;
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[2] = -1;
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[3] = -1;
                }

                //Generating the objects the splits
                for(int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<ArrayTetsData> data = std::make_shared<ArrayTetsData>(
                                arrays_tet.at(i),
                                range_sum.at(i),
                                element_per_items_,
                                true
                                );
                    result.push_back(data);
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
            ConstructTypeSplitPolicy policy = DECAF_SPLIT_DEFAULT)
    {

        std::vector<std::shared_ptr<BaseConstructData> > result;
        // Index of the split for each tet. -1 is no split
        std::vector<int> assigned_split(size_);
        // Number of tets for each split
        std::vector<int> range_sum(range.size(),0);

        switch(policy)
        {
            //We keep only the tets for which we have the 4 particles
            case DECAF_SPLIT_DEFAULT :
            {
                for(int t = 0; t < size_; t++)
                {
                    //Looking for the split of the first particle
                    int split_dest = -1;
                    for(unsigned int i = 0; i < range.size(); i++)
                    {
                        for(unsigned int j = 0; j < range.at(i).size(); j++)
                        {
                            if(value_[t].tets[0] == range.at(i).at(j))
                            {
                                split_dest = i;
                                break;
                            }
                        }

                        //Checking if the other particles are in the same range
                        if(split_dest != -1)
                        {
                            bool part2 = false, part3 = false, part4 = false;
                            for(unsigned int j = 0; j < range.at(i).size(); j++)
                            {
                                if(value_[t].tets[1] == range.at(i).at(j)) part2 = true;
                                else if(value_[t].tets[2] == range.at(i).at(j)) part3 = true;
                                else if(value_[t].tets[3] == range.at(i).at(j)) part4 = true;
                            }

                            //All the particles are in the same range, we can take the tet
                            if(part2 && part3 && part4)
                            {
                                assigned_split[t] = i;
                                range_sum[i] += 1;
                            }

                            //No need to go through the other range
                            break;
                        }
                    }
                }

                //Now we can generate the arrays for the split
                std::vector<int> current_indexes(range.size(),0); //To keep track where to write the tets
                std::vector<tet_t*> arrays_tet;

                //Initializing the arrays
                for(int i = 0; i < range.size(); i++)
                    arrays_tet.push_back(new tet_t[range_sum.at(i)]);

                //Writting the tets
                for(int i = 0; i < size_; i++)
                {
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[0] = value_[i].verts[0];
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[1] = value_[i].verts[1];
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[2] = value_[i].verts[2];
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].verts[3] = value_[i].verts[3];

                    //For now we don't keep the neighors
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[0] = -1;
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[1] = -1;
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[2] = -1;
                    arrays_tet.at(assigned_split[i])[current_indexes.at(assigned_split[i])].tets[3] = -1;
                }

                //Generating the objects the splits
                for(int i = 0; i < range.size(); i++)
                {
                    std::shared_ptr<ArrayTetsData> data = std::make_shared<ArrayTetsData>(
                                arrays_tet.at(i),
                                range_sum.at(i),
                                element_per_items_,
                                true
                                );
                    result.push_back(data);
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

    virtual bool merge(std::shared_ptr<BaseConstructData> other,
                       mapConstruct partial_map,
                       ConstructTypeMergePolicy policy = DECAF_MERGE_DEFAULT)
    {
        //Hypothesis : we assume that we merge the tets before the particles

        //To merge the tet, we have to shift all the references to the particles
        //by the current number of particles and all the references of the tets

        std::shared_ptr<ArrayTetsData> otherArray = std::dynamic_pointer_cast<ArrayTetsData >(other);
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
                std::map<std::string, datafield>::iterator field = partial_map->find("num_particles");
                if(field == partial_map->end())
                {
                    std::cerr<<"ERROR : unable to find the field num_particles which is required "
                            <<"to merge the field of type ArrayTetsData."<<std::endl;
                    return false;
                }

                std::shared_ptr<SimpleConstructData<int> > numParticules =
                        std::dynamic_pointer_cast<SimpleConstructData<int> >(std::get<3>(field->second));
                if(!numParticules)
                {
                    std::cerr<<"ERROR : unable to cast the field num_particles with the type "
                            <<"SimpleConstructData<int> during the merge of a type ArrayTetsData."<<std::endl;
                    return false;
                }
                localNbParticles = numParticules->getData();

                // Number of tets currently in the field. Use to shift the tets indexes
                int localNbTets = size_;

                // Updating the informations of the tets in the other
                tet_t* other_tets = otherArray->getArray();
                int otherNbTets = otherArray->getSize(); // getNbItems is the number of semantic items -> nbParticles

                std::cout<<"Shifting the particles by "<<localNbParticles<<std::endl;
                for(int i = 0; i < otherNbTets; i++)
                {
                    //Shifting the particle indexes
                    other_tets[i].verts[0] = other_tets[i].verts[0] + localNbParticles;
                    other_tets[i].verts[1] = other_tets[i].verts[1] + localNbParticles;
                    other_tets[i].verts[2] = other_tets[i].verts[2] + localNbParticles;
                    other_tets[i].verts[3] = other_tets[i].verts[3] + localNbParticles;

                    //Shifting the tets indexes
                    if(other_tets[i].tets[0] != -1)
                        other_tets[i].tets[0] = other_tets[i].tets[0] + localNbTets;
                    if(other_tets[i].tets[1] != -1)
                        other_tets[i].tets[1] = other_tets[i].tets[1] + localNbTets;
                    if(other_tets[i].tets[2] != -1)
                        other_tets[i].tets[2] = other_tets[i].tets[2] + localNbTets;
                    if(other_tets[i].tets[3] != -1)
                        other_tets[i].tets[3] = other_tets[i].tets[3] + localNbTets;
                }

                //Now we can simply concatenate the arrays
                tet_t* new_tets = new tet_t[size_ + otherNbTets];
                memcpy(new_tets, value_, size_ * sizeof(tet_t));
                memcpy(new_tets + size_, other_tets, otherNbTets * sizeof(tet_t));

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
                std::cout<<"ERROR : policy "<<policy<<" not available for ArrayTetsData."<<std::endl;
                return false;
                break;
            }
        }

    }

};

} //namespace

BOOST_CLASS_EXPORT_GUID(decaf::ArrayTetsData,"ArrayTetsData")

#endif

