#ifndef DECAF_BLOCK_HPP
#define DECAF_BLOCK_HPP

#include <vector>
#include <math.h>
#include <iostream>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/serialization.hpp>

namespace decaf {

template<int Dim>
class Block {

public:
    Block() : hasGridspace_(false), hasGlobalBBox_(false), hasGlobalExtends_(false),
        hasLocalBBox_(false), hasLocalExtends_(false), ghostSize_(0),
        hasOwnBBox_(false), hasOwnExtends_(false)
    {
        globalExtends_.resize(2*Dim);
        localExtends_.resize(2*Dim);
        ownExtends_.resize(2*Dim);

        globalBBox_.resize(2*Dim);
        localBBox_.resize(2*Dim);
        ownBBox_.resize(2*Dim);
    }


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

    void setGhostSizes(unsigned int ghostSize)
    {
        ghostSize_ = ghostSize;
    }

    void setGlobalBBox(std::vector<float>& globalBBox)
    {
        assert(globalBBox.size() == Dim * 2);
        globalBBox_ =  globalBBox;
        hasGlobalBBox_ = true;
    }

    void setGlobalExtends(std::vector<unsigned int>& globalExtends)
    {
        assert(globalExtends.size() == Dim * 2);
        globalExtends_ =  globalExtends;
        hasGlobalExtends_ = true;
    }

    void setLocalBBox(std::vector<float>& localBBox)
    {
        assert(localBBox.size() == Dim * 2);
        localBBox_ = localBBox;
        hasLocalBBox_ = true;
    }

    void setLocalExtends(std::vector<unsigned int>& localExtends)
    {
        assert(localExtends.size() == Dim * 2);
        localExtends_ = localExtends;
        hasLocalExtends_ = true;
    }

    void setOwnBBox(std::vector<float>& ownBBox)
    {
        assert(ownBBox.size() == Dim * 2);
        ownBBox_ = ownBBox;
        hasOwnBBox_ = true;
    }

    void setOwnExtends(std::vector<unsigned int>& ownExtends)
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
            globalExtends_.resize(2*Dim);
            for(unsigned int i = 0; i < Dim; i++)
            {
                globalExtends_[i] = 0;
                //Ceil because we count the number of cells
                globalExtends_[Dim+i] = (unsigned int)(ceil(globalBBox_[Dim+i] / gridspace_));
            }
            hasGlobalExtends_ = true;

        }

        if(hasLocalBBox_)
        {
            localExtends_.resize(2*Dim);
            for(unsigned int i = 0; i < Dim; i++)
            {
                // TODO : here should be another value because of ghost region
                localExtends_[i] = 0;
                //Ceil because we count the number of cells
                localExtends_[Dim+i] = (unsigned int)(ceil(localBBox_[Dim+i] / gridspace_));
            }
            hasLocalExtends_ = true;
        }

        if(hasOwnBBox_)
        {
            ownExtends_.resize(2*Dim);
            for(unsigned int i = 0; i < Dim; i++)
            {
                // TODO : here should be another value because of ghost region
                ownExtends_[i] = 0;
                //Ceil because we count the number of cells
                ownExtends_[Dim+i] = (unsigned int)(ceil(ownBBox_[Dim+i] / gridspace_));
            }
            hasOwnExtends_ = true;
        }

        return true;
    }

    bool isInGlobalBlock(float x, float y, float z) const
    {
        if(!hasGlobalBBox_) return false;
        return x >= globalBBox_[0] && x <= globalBBox_[0] + globalBBox_[3] &&
                y >= globalBBox_[1] && y <= globalBBox_[1] + globalBBox_[4] &&
                z >= globalBBox_[2] && z <= globalBBox_[2] + globalBBox_[5];
    }

    bool isInLocalBlock(float x, float y, float z) const
    {
        if(!hasLocalBBox_) return false;
        return x >= localBBox_[0] && x <= localBBox_[0] + localBBox_[3] &&
                y >= localBBox_[1] && y <= localBBox_[1] + localBBox_[4] &&
                z >= localBBox_[2] && z <= localBBox_[2] + localBBox_[5];
    }

    bool isInOwnBlock(float x, float y, float z) const
    {
        if(!hasOwnBBox_) return false;
        return x >= ownBBox_[0] && x <= ownBBox_[0] + ownBBox_[3] &&
                y >= ownBBox_[1] && y <= ownBBox_[1] + ownBBox_[4] &&
                z >= ownBBox_[2] && z <= ownBBox_[2] + ownBBox_[5];
    }

    bool isInGlobalBlock(unsigned int x, unsigned int y, unsigned int z) const
    {
        if(!hasGlobalExtends_) return false;
        return x >= globalExtends_[0] && x < globalExtends_[0] + globalExtends_[3] &&
                y >= globalExtends_[1] && y < globalExtends_[1] + globalExtends_[4] &&
                z >= globalExtends_[2] && z < globalExtends_[2] + globalExtends_[5];
    }

    bool isInLocalBlock(unsigned int x, unsigned int y, unsigned int z) const
    {
        if(!hasLocalExtends_) return false;
        return x >= localExtends_[0] && x < localExtends_[0] + localExtends_[3] &&
                y >= localExtends_[1] && y < localExtends_[1] + localExtends_[4] &&
                z >= localExtends_[2] && z < localExtends_[2] + localExtends_[5];
    }

    bool isInOwnBlock(unsigned int x, unsigned int y, unsigned int z) const
    {
        if(!hasOwnExtends_) return false;
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

    bool doExtendsInclude(const Block<Dim> & other)
    {
        if(hasGlobalExtends_ && other.hasGlobalExtends_)
        {
            for(unsigned int i = 0; i < Dim; i++)
                if(globalExtends_[i] > other.globalExtends_[i])
                    return false;
            for(unsigned int i = Dim; i < 2*Dim; i++)
                if(globalExtends_[i-Dim]+globalExtends_[i] < other.globalExtends_[i-Dim] + other.globalExtends_[i])
                    return false;
        }

        if(hasLocalExtends_ && other.hasLocalExtends_)
        {
            for(unsigned int i = 0; i < Dim; i++)
                if(localExtends_[i] > other.localExtends_[i])
                    return false;
            for(unsigned int i = Dim; i < 2*Dim; i++)
                if(localExtends_[i-Dim]+localExtends_[i] < other.localExtends_[i-Dim] + other.localExtends_[i])
                    return false;
        }

        if(hasOwnExtends_ && other.hasOwnExtends_)
        {
            for(unsigned int i = 0; i < Dim; i++)
                if(ownExtends_[i] > other.ownExtends_[i])
                    return false;
            for(unsigned int i = Dim; i < 2*Dim; i++)
                if(ownExtends_[i-Dim]+ownExtends_[i] < other.ownExtends_[i-Dim] + other.ownExtends_[i])
                    return false;
        }

        return true;
    }

    void makeUnion(const Block<Dim> & other)
    {
        makeExtendsUnion(other);
        makeBBoxUnion(other);
    }

    void makeUnion(const Block<Dim> & other, Block<Dim> & result)
    {
        makeExtendsUnion(other, result);
        makeBBoxUnion(other, result);
    }

    void makeExtendsUnion(const Block<Dim> & other)
    {
        for(unsigned i = 0; i < Dim; i++)
        {
            if(hasGlobalExtends_ && other.hasGlobalExtends_)
            {
                unsigned int origin = std::min(globalExtends_[i], other.globalExtends_[i]);
                unsigned int delta = std::max(
                            globalExtends_[i]+globalExtends_[Dim+i],
                            other.globalExtends_[i]+other.globalExtends_[Dim+i]
                            ) - origin;
                globalExtends_[i] = origin;
                globalExtends_[Dim+i] = delta;
            }

            if(hasLocalExtends_ && other.hasLocalExtends_)
            {
                unsigned int origin = std::min(localExtends_[i], other.localExtends_[i]);
                unsigned int delta = std::max(
                            localExtends_[i]+localExtends_[Dim+i],
                            other.localExtends_[i]+other.localExtends_[Dim+i]
                            ) - origin;
                localExtends_[i] = origin;
                localExtends_[Dim+i] = delta;
            }

            if(hasOwnExtends_ && other.hasOwnExtends_)
            {
                unsigned int origin = std::min(ownExtends_[i], other.ownExtends_[i]);
                unsigned int delta = std::max(
                            ownExtends_[i]+ownExtends_[Dim+i],
                            other.ownExtends_[i]+other.ownExtends_[Dim+i]
                            ) - origin;
                ownExtends_[i] = origin;
                ownExtends_[Dim+i] = delta;
            }

        }
    }

    // Used during the merge
    void makeBBoxUnion(const Block<Dim> & other)
    {
        for(unsigned i = 0; i < Dim; i++)
        {
            if(hasGlobalBBox_ && other.hasGlobalBBox_)
            {
                float dMax = std::max(globalBBox_[i] + globalBBox_[Dim+i], other.globalBBox_[i] + other.globalBBox_[Dim+i]);
                globalBBox_[i] = std::min(globalBBox_[i], other.globalBBox_[i]);
                globalBBox_[Dim+i] = dMax - globalBBox_[i];
            }

            if(hasLocalBBox_ && other.hasLocalBBox_)
            {
                float dMax = std::max(localBBox_[i] + localBBox_[Dim+i], other.localBBox_[i] + other.localBBox_[Dim+i]);
                localBBox_[i] = std::min(localBBox_[i], other.localBBox_[i]);
                localBBox_[Dim+i] = dMax - localBBox_[i];
            }

            if(hasOwnBBox_ && other.hasOwnBBox_)
            {
                float dMax = std::max(ownBBox_[i] + ownBBox_[Dim+i], other.ownBBox_[i] + other.ownBBox_[Dim+i]);
                ownBBox_[i] = std::min(ownBBox_[i], other.ownBBox_[i]);
                ownBBox_[Dim+i] = dMax - ownBBox_[i];
            }
        }
    }

    void makeExtendsUnion(const Block<Dim> & other, Block<Dim> & result)
    {
        for(unsigned i = 0; i < Dim; i++)
        {
            if(hasGlobalExtends_ && other.hasGlobalExtends_)
            {
                unsigned int origin = std::min(globalExtends_[i], other.globalExtends_[i]);
                unsigned int delta = std::max(
                            globalExtends_[i]+globalExtends_[Dim+i],
                            other.globalExtends_[i]+other.globalExtends_[Dim+i]
                            ) - origin;
                result.hasGlobalExtends_ = true;
                result.globalExtends_[i] = origin;
                result.globalExtends_[Dim+i] = delta;
            }

            if(hasLocalExtends_ && other.hasLocalExtends_)
            {
                unsigned int origin = std::min(localExtends_[i], other.localExtends_[i]);
                unsigned int delta = std::max(
                            localExtends_[i]+localExtends_[Dim+i],
                            other.localExtends_[i]+other.localExtends_[Dim+i]
                            ) - origin;
                result.hasLocalExtends_ = true;
                result.localExtends_[i] = origin;
                result.localExtends_[Dim+i] = delta;
            }

            if(hasOwnExtends_ && other.hasOwnExtends_)
            {
                unsigned int origin = std::min(ownExtends_[i], other.ownExtends_[i]);
                unsigned int delta = std::max(
                            ownExtends_[i]+ownExtends_[Dim+i],
                            other.ownExtends_[i]+other.ownExtends_[Dim+i]
                            ) - origin;
                result.hasOwnExtends_ = true;
                result.ownExtends_[i] = origin;
                result.ownExtends_[Dim+i] = delta;
            }
        }
    }

    void makeBBoxUnion(const Block<Dim> & other, Block<Dim> & result)
    {
        for(unsigned i = 0; i < Dim; i++)
        {
            if(hasGlobalBBox_ && other.hasGlobalBBox_)
            {
                result.hasGlobalBBox_ = true;
                float dMax = std::max(globalBBox_[i] + globalBBox_[Dim+i], other.globalBBox_[i] + other.globalBBox_[Dim+i]);
                result.globalBBox_[i] = std::min(globalBBox_[i], other.globalBBox_[i]);
                result.globalBBox_[Dim+i] = dMax - globalBBox_[i];
            }

            if(hasLocalBBox_ && other.hasLocalBBox_)
            {
                result.hasLocalBBox_ = true;
                float dMax = std::max(localBBox_[i] + localBBox_[Dim+i], other.localBBox_[i] + other.localBBox_[Dim+i]);
                result.localBBox_[i] = std::min(localBBox_[i], other.localBBox_[i]);
                result.localBBox_[Dim+i] = dMax - localBBox_[i];
            }

            if(hasOwnBBox_ && other.hasOwnBBox_)
            {
                result.hasOwnBBox_ = true;
                float dMax = std::max(ownBBox_[i] + ownBBox_[Dim+i], other.ownBBox_[i] + other.ownBBox_[Dim+i]);
                result.ownBBox_[i] = std::min(ownBBox_[i], other.ownBBox_[i]);
                result.ownBBox_[Dim+i] = dMax - ownBBox_[i];
            }
        }
    }

    // Expect the position in global space
    // Return the position in the own index
    bool getOwnPositionIndex(float* pos, unsigned int* index)
    {
        // TODO : test if the position is in the space

        // Move the absolute coordinates to the own coordinates
        for(unsigned int i = 0; i <  Dim; i++)
        {
            float offsetPos = pos[i] - ownBBox_[i];
            index[i] = (unsigned int)(floor(offsetPos / gridspace_));
        }

        return true;
    }

    // Expect the position index in Own index
    // Return a position in the global space
    bool getOwnPositionValue(unsigned int* index, float* pos)
    {
        assert(hasOwnBBox_);
        for(unsigned int i = 0; i <  Dim; i++)
        {
            // 0.5 to get to the middle of the cell
            pos[i] = ownBBox_[i] + ((float)index[i] + 0.5) * gridspace_;
        }
        return true;
    }

    // Expect the position in global space
    // Return the position in the local index
    bool getLocalPositionIndex(float* pos, unsigned int* index)
    {
        // TODO : test if the position is in the space

        // Move the absolute coordinates to the own coordinates
        for(unsigned int i = 0; i <  Dim; i++)
        {
            float offsetPos = pos[i] - localBBox_[i];
            index[i] = (unsigned int)(floor(offsetPos / gridspace_));
        }

        return true;
    }

    // Expect the position index in Local index
    // Return a position in the global space
    bool getLocalPositionValue(unsigned int* index, float* pos)
    {
        assert(hasLocalBBox_);
        for(unsigned int i = 0; i <  Dim; i++)
        {
            // 0.5 to get to the middle of the cell
            pos[i] = localBBox_[i] + ((float)index[i] + 0.5) * gridspace_;
        }
        return true;
    }

    // Expect the position in global space
    // Return the position in the local index
    bool getGlobalPositionIndex(float* pos, unsigned int* index)
    {
        // TODO : test if the position is in the space

        // Move the absolute coordinates to the own coordinates
        for(unsigned int i = 0; i <  Dim; i++)
        {
            float offsetPos = pos[i] - globalBBox_[i];
            index[i] = (unsigned int)(floor(offsetPos / gridspace_));
        }

        return true;
    }

    // Expect the position index in Global index
    // Return a position in the global space
    bool getGlobalPositionValue(unsigned int* index, float* pos)
    {
        assert(hasGlobalBBox_);
        for(unsigned int i = 0; i <  Dim; i++)
        {
            // 0.5 to get to the middle of the cell
            pos[i] = globalBBox_[i] + ((float)index[i] + 0.5) * gridspace_;
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
        for(unsigned int i = 0; i < Dim; i++)
        {
            if(localExtends_[i] >= ghostSize_)
                //We can safely extend the region
                localExtends_[i] -= ghostSize_;
            else
                localExtends_[i] = 0;
        }

        //Checking if with the ghost region we don't go outside of the global box
        for(unsigned int i = Dim; i < 2*Dim; i++)
        {
            if(localExtends_[i-Dim] + localExtends_[i] + ghostSize_ <= globalExtends_[i])
                localExtends_[i] += ghostSize_;
            else
                localExtends_[i] += globalExtends_[i] - localExtends_[i-Dim] - localExtends_[i];
        }

        for(unsigned int j = 0; j < Dim; j++)
            localBBox_[j] = globalBBox_[j] + (float)localExtends_[j] * gridspace_;
        for(unsigned int j = Dim; j < 2*Dim; j++)
            localBBox_[j] = (float)(localExtends_[j]) * gridspace_;

    }

    bool hasGhostRegions(){ return ghostSize_ > 0; }

    void printExtend(const std::vector<unsigned int>& extend) const
    {
        std::cout<<"["<<extend[0]<<","<<extend[1]<<","<<extend[2]<<"]"
                <<"["<<extend[0]+extend[3]<<","<<extend[1]+extend[4]
                <<","<<extend[2]+extend[5]<<"]"<<std::endl;
    }

    void printBox(const std::vector<float>& box) const
    {
        std::cout<<"["<<box[0]<<","<<box[1]<<","<<box[2]<<"]"
                <<"["<<box[0]+box[3]<<","<<box[1]+box[4]<<","
                                                       <<box[2]+box[5]<<"]"<<std::endl;
    }

    void printBoxes() const
    {
        if(hasGlobalBBox_)
        {
            std::cout<<"Global bounding box : ";
            printBox(globalBBox_);
        }
        if(hasLocalBBox_)
        {
            std::cout<<"Local bounding box : ";
            printBox(localBBox_);
        }
        if(hasOwnBBox_)
        {
            std::cout<<"Own bounding box : ";
            printBox(ownBBox_);
        }
    }

    void printExtends() const
    {
        if(hasGlobalExtends_)
        {
            std::cout<<"Global Extends : ";
            printExtend(globalExtends_);
        }
        if(hasLocalExtends_)
        {
            std::cout<<"Local Extends : ";
            printExtend(localExtends_);
        }
        if(hasOwnExtends_)
        {
            std::cout<<"Own Extends : ";
            printExtend(ownExtends_);
        }

    }

    float* getGlobalBBox()
    {
        if(hasGlobalBBox_)
            return &globalBBox_[0];
        else
            return NULL;
    }

    float* getLocalBBox()
    {
        if(hasLocalBBox_)
            return &localBBox_[0];
        else
            return NULL;
    }

    float* getOwnBBox()
    {
        if(hasOwnBBox_)
            return &ownBBox_[0];
        else
            return NULL;
    }

    unsigned int* getGlobalExtends()
    {
        if(hasGlobalExtends_)
            return &globalExtends_[0];
        else
            return NULL;
    }

    unsigned int* getLocalExtends()
    {
        if(hasLocalExtends_)
            return &localExtends_[0];
        else
            return NULL;
    }

    unsigned int* getOwnExtends()
    {
        if(hasOwnExtends_)
            return &ownExtends_[0];
        else
            return NULL;
    }

    float getGridspace()
    {
        if(hasGridspace_)
            return gridspace_;
        else
            return 0.0f;
    }

    unsigned int getGhostsize()
    {
        return ghostSize_;
    }


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

#endif
