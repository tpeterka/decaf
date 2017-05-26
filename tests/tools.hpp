#ifndef TEST_TOOLS_HPP
#define TEST_TOOLS_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include <bredala/data_model/vectorconstructdata.hpp>
#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/pconstructtype.h>

using namespace decaf;
using namespace std;

void getHeatMapColor(float value, float *red, float *green, float *blue)
{
    const int NUM_COLORS = 4;
    static double color[NUM_COLORS][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };
    // A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.

    int idx1;        // |-- Our desired color will be between these two indexes in "color".
    int idx2;        // |
    double fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

    if(value <= 0)       {  idx1 = idx2 = 0;            }    // accounts for an input <=0
    else if(value >= 1)  {  idx1 = idx2 = NUM_COLORS-1; }    // accounts for an input >=0
    else
    {
        value = value * (NUM_COLORS-1);        // Will multiply value by 3.
        idx1  = floor(value);                  // Our desired color will be after this index.
        idx2  = idx1+1;                        // ... and before this index (inclusive).
        fractBetween = value - double(idx1);   // Distance between the two indexes (0-1).
    }

    *red   = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
    *green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
    *blue  = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
    return std::max(lower, std::min(n, upper));
}

void posToFile(const float* pos, int nbParticles, const string filename, unsigned int r, unsigned int g, unsigned int b)
{
    ofstream file;
    cout<<"Filename : "<<filename<<endl;
    file.open(filename.c_str());

    unsigned int ur,ug,ub;
    ur = clip<unsigned int>(r, 0, 255);
    ug = clip<unsigned int>(g, 0, 255);
    ub = clip<unsigned int>(b, 0, 255);
    cout<<"UColor : "<<ur<<","<<ug<<","<<ub<<endl;

    cout<<"Number of particules to save : "<<nbParticles<<endl;
    file<<"ply"<<endl;
    file<<"format ascii 1.0"<<endl;
    file<<"element vertex "<<nbParticles<<endl;
    file<<"property float x"<<endl;
    file<<"property float y"<<endl;
    file<<"property float z"<<endl;
    file<<"property uchar red"<<endl;
    file<<"property uchar green"<<endl;
    file<<"property uchar blue"<<endl;
    file<<"end_header"<<endl;
    for(int i = 0; i < nbParticles; i++)
        file<<pos[3*i]<<" "<<pos[3*i+1]<<" "<<pos[3*i+2]
            <<" "<<ur<<" "<<ug<<" "<<ub<<endl;
    file.close();
}

void posToFile(const std::vector<float>& pos, const std::string filename, unsigned int r, unsigned int g, unsigned int b)
{
    posToFile(&pos[0], pos.size() / 3, filename, r, g, b);
}



void printMap(ConstructData& map)
{
    std::shared_ptr<VectorConstructData<float> > array =
        dynamic_pointer_cast<VectorConstructData<float> >(map.getData("pos"));
    std::shared_ptr<SimpleConstructData<int> > data =
        dynamic_pointer_cast<SimpleConstructData<int> >(map.getData("nbParticles"));

    std::cout<<"Number of particle : "<<data->getData()<<std::endl;
    std::cout<<"Positions : "<<std::endl;
    for(unsigned int i = 0; i < array->getVector().size() / 3; i++)
    {
        std::cout<<"["<<array->getVector().at(3*i)
                 <<","<<array->getVector().at(3*i+1)
                 <<","<<array->getVector().at(3*i+2)<<"]"<<std::endl;
    }
}

void initPosition(float* pos, int nbParticle, const vector<float>& bbox)
{
    float dx = bbox[3] - bbox[0];
    float dy = bbox[4] - bbox[1];
    float dz = bbox[5] - bbox[2];

    for(int i = 0;i < nbParticle; i++)
    {
        pos[3*i]   = (bbox[0] + ((float)rand() / (float)RAND_MAX) * dx);
        pos[3*i+1] = (bbox[1] + ((float)rand() / (float)RAND_MAX) * dy);
        pos[3*i+2] = (bbox[2] + ((float)rand() / (float)RAND_MAX) * dz);
    }
}

void initPosition(std::vector<float>& pos, int nbParticle, const std::vector<float>& bbox)
{
    pos.resize(3*nbParticle);
    initPosition(&pos[0], nbParticle, bbox);
}



bool isBetween(int rank, int start, int nb)
{
    return rank >= start && rank < start + nb;
}

void printArray(const std::vector<int>& array)
{
    std::cout<<"[";
    for(unsigned int i = 0; i < array.size(); i++)
        std::cout<<array[i]<<",";
    std::cout<<"]"<<std::endl;
}

#endif
