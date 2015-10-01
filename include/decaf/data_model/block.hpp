#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <vector>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

namespace decaf {

void printExtends(std::vector<unsigned int>& extend)
{
    std::cout<<"["<<extend[0]<<","<<extend[1]<<","<<extend[2]<<"]"
             <<"["<<extend[0]+extend[3]<<","<<extend[1]+extend[4]<<","<<extend[2]+extend[5]<<"]"<<std::endl;
}

template<int Dim>
class Block {

public:
    Block() : hasGridspace_(false), hasGlobalBBox_(false), hasGlobalExtends_(false),
              hasLocalBBox_(false), hasLocalExtends_(false), ghostSize_(0),
              hasOwnBBox_(false), hasOwnExtends_(false){}

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(hasGridspace_);
        ar & BOOST_SERIALIZATION_NVP(gridspace_);
        ar & BOOST_SERIALIZATION_NVP(hasGlobalBBox_);
        ar & BOOST_SERIALIZATION_NVP(globalBBox_);
        ar & BOOST_SERIALIZATION_NVP(hasGlobalExtends_);
        ar & BOOST_SERIALIZATION_NVP(globalExtends_);
        ar & BOOST_SERIALIZATION_NVP(hasLocalBBox_);
        ar & BOOST_SERIALIZATION_NVP(localBBox_);
        ar & BOOST_SERIALIZATION_NVP(hasLocalExtends_);
        ar & BOOST_SERIALIZATION_NVP(localExtends_);
        ar & BOOST_SERIALIZATION_NVP(ghostSize_);
        ar & BOOST_SERIALIZATION_NVP(hasOwnBBox_);
        ar & BOOST_SERIALIZATION_NVP(ownBBox_);
        ar & BOOST_SERIALIZATION_NVP(hasOwnExtends_);
        ar & BOOST_SERIALIZATION_NVP(ownExtends_);
    }

    void setGridspace(float gridspace)
    {
        gridspace_ = gridspace;
        hasGridspace_ = true;
    }

    void setGlobalBBox(std::vector<float> globalBBox)
    {
        assert(globalBBox.size() == Dim * 2);
        globalBBox_ =  globalBBox;
        hasGlobalBBox_ = true;
    }

    void setGlobalExtends(std::vector<unsigned int> globalExtends)
    {
        assert(globalExtends.size() == Dim * 2);
        globalExtends_ =  globalExtends;
        hasGlobalExtends_ = true;
    }

    void setLocalBBox(std::vector<float> localBBox)
    {
        assert(localBBox.size() == Dim * 2);
        localBBox_ = localBBox;
        hasLocalBBox_ = true;
    }

    void setLocalExtends(std::vector<unsigned int> localExtends)
    {
        assert(localExtends.size() == Dim * 2);
        localExtends_ = localExtends;
        hasLocalExtends_ = true;
    }

    void setOwnBBox(std::vector<float> ownBBox)
    {
        assert(ownBBox.size() == Dim * 2);
        ownBBox_ = ownBBox;
        hasOwnBBox_ = true;
    }

    void setOwnExtends(std::vector<unsigned int> ownExtends)
    {
        assert(ownExtends.size() == Dim * 2);
        ownExtends_ = ownExtends;
        hasOwnExtends_ = true;
    }

    bool updateExtends()
    {
        if(!hasGridspace_)
            return false;
        if(hasGlobalBBox_)
        {
            globalExtends_.resize(6);
            globalExtends_[0] = 0;
            globalExtends_[1] = 0;
            globalExtends_[2] = 0;
            //Ceil because we count the number of cells
            globalExtends_[3] = (unsigned int)(ceil(globalBBox_[3] / gridspace_));
            globalExtends_[4] = (unsigned int)(ceil(globalBBox_[4] / gridspace_));
            globalExtends_[5] = (unsigned int)(ceil(globalBBox_[5] / gridspace_));
            hasGlobalExtends_ = true;

        }

        if(hasLocalBBox_)
        {
            localExtends_.resize(6);
            localExtends_[0] = (unsigned int)(floor(localBBox_[0] / gridspace_));
            localExtends_[1] = (unsigned int)(floor(localBBox_[1] / gridspace_));
            localExtends_[2] = (unsigned int)(floor(localBBox_[2] / gridspace_));
            //Ceil because we count the number of cells
            localExtends_[3] = (unsigned int)(ceil(localBBox_[3] / gridspace_));
            localExtends_[4] = (unsigned int)(ceil(localBBox_[4] / gridspace_));
            localExtends_[5] = (unsigned int)(ceil(localBBox_[5] / gridspace_));
            hasLocalExtends_ = true;
        }

        if(hasOwnBBox_)
        {
            ownExtends_.resize(6);
            ownExtends_[0] = (unsigned int)(floor(ownBBox_[0] / gridspace_));
            ownExtends_[1] = (unsigned int)(floor(ownBBox_[1] / gridspace_));
            ownExtends_[2] = (unsigned int)(floor(ownBBox_[2] / gridspace_));
            //Ceil because we count the number of cells
            ownExtends_[3] = (unsigned int)(ceil(ownBBox_[3] / gridspace_));
            ownExtends_[4] = (unsigned int)(ceil(ownBBox_[4] / gridspace_));
            ownExtends_[5] = (unsigned int)(ceil(ownBBox_[5] / gridspace_));
            hasOwnExtends_ = true;
        }

        return true;
    }

    bool isInGlobalBlock(float x, float y, float z) const
    {
        return x >= globalBBox_[0] && x <= globalBBox_[0] + globalBBox_[3] &&
               y >= globalBBox_[1] && y <= globalBBox_[1] + globalBBox_[4] &&
               z >= globalBBox_[2] && z <= globalBBox_[2] + globalBBox_[5];
    }

    bool isInLocalBlock(float x, float y, float z) const
    {
        return x >= localBBox_[0] && x <= localBBox_[0] + localBBox_[3] &&
               y >= localBBox_[1] && y <= localBBox_[1] + localBBox_[4] &&
               z >= localBBox_[2] && z <= localBBox_[2] + localBBox_[5];
    }

    bool isInOwnBlock(float x, float y, float z) const
    {
        return x >= ownBBox_[0] && x <= ownBBox_[0] + ownBBox_[3] &&
               y >= ownBBox_[1] && y <= ownBBox_[1] + ownBBox_[4] &&
               z >= ownBBox_[2] && z <= ownBBox_[2] + ownBBox_[5];
    }

    bool isInGlobalBlock(unsigned int x, unsigned int y, unsigned int z) const
    {
        return x >= globalExtends_[0] && x < globalExtends_[0] + globalExtends_[3] &&
               y >= globalExtends_[1] && y < globalExtends_[1] + globalExtends_[4] &&
               z >= globalExtends_[2] && z < globalExtends_[2] + globalExtends_[5];
    }

    bool isInLocalBlock(unsigned int x, unsigned int y, unsigned int z) const
    {
        return x >= localExtends_[0] && x < localExtends_[0] + localExtends_[3] &&
               y >= localExtends_[1] && y < localExtends_[1] + localExtends_[4] &&
               z >= localExtends_[2] && z < localExtends_[2] + localExtends_[5];
    }

    bool isInOwnBlock(unsigned int x, unsigned int y, unsigned int z) const
    {
        return x >= ownExtends_[0] && x < ownExtends_[0] + ownExtends_[3] &&
               y >= ownExtends_[1] && y < ownExtends_[1] + ownExtends_[4] &&
               z >= ownExtends_[2] && z < ownExtends_[2] + ownExtends_[5];
    }

    bool hasSameExtends(const Block<Dim> & other)
    {
        if(hasGlobalExtends_ && other.hasGlobalExtends_)
        {
            for(unsigned int i = 0; i < 2 * Dim; i++)
                if(globalExtends_[i] != other.globalExtends_[i])
                    return false;
        }

        if(hasLocalExtends_ && other.hasLocalExtends_)
        {
            for(unsigned int i = 0; i < 2 * Dim; i++)
                if(localExtends_[i] != other.localExtends_[i])
                    return false;
        }

        if(hasOwnExtends_ && other.hasOwnExtends_)
        {
            for(unsigned int i = 0; i < 2 * Dim; i++)
                if(ownExtends_[i] != other.ownExtends_[i])
                    return false;
        }

        return true;
    }

    void buildGhostRegions(unsigned int ghostSize)
    {
        assert(hasLocalExtends_ && hasLocalBBox_);

        ghostSize_ = ghostSize;

        //The ownBox is becomming is becomming the localBox and we extend
        //the localBox with the ghost region

        setOwnBBox(localBBox_);
        setOwnExtends(localExtends_);

        //Updating the info of the localbox
        //Checking the minimum dimension if we don't go bellow 0
        for(unsigned int i = 0; i < 3; i++)
        {
            if(localExtends_[i] >= ghostSize_)
                //We can safely extend the region
                localExtends_[i] -= ghostSize_;
            else
                localExtends_[i] = 0;
        }

        //Checking if with the ghost region we don't go outside of the global box
        for(unsigned int i = 3; i < 6; i++)
        {
            if(localExtends_[i-3] + localExtends_[i] + ghostSize_ <= globalExtends_[i])
                localExtends_[i] += ghostSize_;
            else
                localExtends_[i] += globalExtends_[i] - localExtends_[i-3] - localExtends_[i];
        }
    }

    bool hasGhostRegions(){ return ghostSize_ > 0; }


    bool hasGridspace_;                     //Size of a cell
    float gridspace_;

    //Global bounding box of the domain/simulation
    bool hasGlobalBBox_;
    std::vector<float> globalBBox_;         //Min, delta for the complete domain
    bool hasGlobalExtends_;
    std::vector<unsigned int> globalExtends_;//Number of cells in each dimensions

    //Local domain manage withing the process with Ghost region
    bool hasLocalBBox_;
    std::vector<float> localBBox_;          //Min, delta for the localdomain domain with ghost
    bool hasLocalExtends_;
    std::vector<unsigned int> localExtends_;//Start offset + Number of cells in each dimensions with ghost

    //Local domain manage within the process without Ghost region
    unsigned int ghostSize_;                //Width of the ghost region in the 3 dimensions
    bool hasOwnBBox_;
    std::vector<float> ownBBox_;            //Min, delta without ghost region
    bool hasOwnExtends_;
    std::vector<unsigned int> ownExtends_;  //Start offset + Number of cells in each dimensions without ghost

};

}// namespace

BOOST_CLASS_EXPORT_GUID(decaf::Block<1>,"decaf::Block<1>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<2>,"decaf::Block<2>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<3>,"decaf::Block<3>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<4>,"decaf::Block<4>")
BOOST_CLASS_EXPORT_GUID(decaf::Block<5>,"decaf::Block<5>")

#endif
