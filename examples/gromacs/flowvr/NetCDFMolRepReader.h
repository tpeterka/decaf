/******* COPYRIGHT ************************************************
*                                                                 *
*                             Vitamins                            *
*                  Analytics Framework for Molecular Dynamics     *
*                                                                 *
*-----------------------------------------------------------------*
* COPYRIGHT (C) 2014 by                                           *
* INRIA, CNRS, Université d'Orsay, Université d'Orléans           *
* ALL RIGHTS RESERVED.                                            *
*                                                                 *
* This source is covered by the CeCILL-C licence,                 *
* Please refer to the COPYING file for further information.       *
*                                                                 *
*-----------------------------------------------------------------*
*                                                                 *
*  Original Contributors:                                         *
*  Marc Baaden                                                    *
*  Matthieu Dreher                                                *
*  Nicolas Férey                                                  *
*  Sébastien Limet                                                *
*  Jessica Prevoteau-Jonquet                                      *
*  Marc Piuzzi                                                    *
*  Millian Poquet                                                 *
*  Bruno Raffin                                                   *
*  Sophie Robert                                                  *
*  Mikaël Trellet                                                 *
*                                                                 *
*******************************************************************
*    Contact :                                                    *
*    Matthieu Dreher <matthieu.dreher@inria.fr                    *
******************************************************************/


#ifndef _NETCDFMOLREPREADER_H_
#define _NETCDFMOLREPREADER_H_

#include <string>

#include "vitamins_data.h"

class NetCDFMolRepReader
{
public :
    NetCDFMolRepReader();
    NetCDFMolRepReader(const std::string & filename);
    virtual ~NetCDFMolRepReader();

    void setFileName(const std::string & filename);
    void read();

    void getSpheres(spheretype * spheres) const;
    void getPositions(float * positions) const;
    void getCylinders(cylindertype * cylinders) const;

    unsigned getSpheresNumber() const;
    unsigned getCylindersNumber() const;

    void setSpheresNumber(unsigned spheresnum) ;
    void setCylindersNumber(unsigned cylindersnum)  ;


    void getCenter(float * center) const ;

    void getBB(BBoxtype* BB_) const ;

    BBoxtype *BBox;

protected :


private :
    std::string _filename;
    spheretype * _spheres;
    cylindertype * _cylinders;
    unsigned _spheresnum;
    unsigned _cylindersnum;
    float * spherecoordinates;
};

#endif
