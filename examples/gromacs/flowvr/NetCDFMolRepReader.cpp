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


#include "NetCDFMolRepReader.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iostream>

#include <netcdfcpp.h>
#include <netcdf.h>

#include <QtCore/QRegExp>
#include <QtCore/QString>

using namespace std;

NetCDFMolRepReader::NetCDFMolRepReader()
{
    _filename="";
    _spheres = 0;
    _cylinders = 0;
    _spheresnum = 0;
    _cylindersnum = 0;
    BBox=(BBoxtype *) malloc (sizeof(BBoxtype ));//  new BBoxtype;
}

NetCDFMolRepReader::~NetCDFMolRepReader()
{
    free(_cylinders);
    free(_spheres);
}


void NetCDFMolRepReader::setFileName(const std::string & filename)
{
    _filename = filename;
    _spheres = 0;
    _cylinders = 0;
    _spheresnum = 0;
    _cylindersnum = 0;
}



void NetCDFMolRepReader::read()
{
    NcFile * file = new NcFile(_filename.c_str());

    std::cout << "fonction read " << _filename.c_str() << std::endl;
    if(file->is_valid())
    {
        unsigned numdims=file->num_dims();
        unsigned numvars=file->num_vars();
        unsigned numatts=file->num_atts();

        /*cout<<"Number of dimensions : "<<numdims<<endl;
        cout<<"Number of variables : "	<<numvars<<endl;
        cout<<"Number of global attributes : "<<numatts<<endl;*/


        int * sphereids=0;
        spherecoordinates=0;
        float * spherecolors=0;
        float * sphereradii=0;
        float * spherescale=0;

        char * spherenames=0;

        unsigned sphere_number=0;
        unsigned cylinder_number=0;

        int * cylinderids=0;

        int * cylinderrefs=0;
        float * cylindercolors = 0;

        float * cylindershrink=0;
        float * cylinderscale=0;

        unsigned spatialdim = 3;
        unsigned colordim = 4;
        unsigned cylinderdim = 2;

        unsigned spherename_length = 20;


        for(unsigned i=0;i<numdims;i++)
        {
            NcDim * dim=file->get_dim(i);
            //cout<<"Dimension : "<<dim->name()<<" "<<dim->size()<<endl;
            if(strcmp(dim->name(),"spatialdim")==0)
                spatialdim=dim->size();
            else if(strcmp(dim->name(),"colordim")==0)
                colordim=dim->size();
            else if(strcmp(dim->name(),"cylinderdim")==0)
                cylinderdim=dim->size();
            else if(strcmp(dim->name(),"sphere_number")==0)
                sphere_number=dim->size();
            else if(strcmp(dim->name(),"cylinder_number")==0)
                cylinder_number=dim->size();
            else if(strcmp(dim->name(),"cylinder_number")==0)
                cylinder_number=dim->size();
        }



        for(unsigned i=0;i<numvars;i++)
        {
            NcVar * var=file->get_var(i);
            //cout<<"Variable : "<<var->name()<<" "<<endl;
            if(strcmp(var->name(),"sphereids") == 0)
            {
                sphereids = (int *) var->values()->base();
                //cout<<"sphereids : "<<endl;
                /*for(unsigned j=0;j< sphere_number  ;j++)
                    {
                    cout << " "<<sphereids[j]<<endl;
                    }*/
            }
            else if(strcmp(var->name(),"spherecoordinates") == 0)
            {
                spherecoordinates = (float *) var->values()->base();
                //cout<<"spherecoordinates : "<<endl;
                BBox->Xmin =  spherecoordinates[0];
                BBox->Ymin =  spherecoordinates[1];
                BBox->Zmin =  spherecoordinates[2];
                BBox->Xmax =  spherecoordinates[0];
                BBox->Ymax =  spherecoordinates[1];
                BBox->Zmax =  spherecoordinates[2];

		std::cout<<"Boite englobante : ("<<BBox->Xmin<<","<<BBox->Ymin<<","<<BBox->Zmin<<") ("<<BBox->Xmax<<","<<BBox->Ymax<<","<<BBox->Zmax<<")"<<std::endl;

                for(unsigned j = 0; j < sphere_number; j++)
                {
#ifdef VERBOSE
                    cout<<spherecoordinates[j*3]<<" "<<spherecoordinates[j*3+1]<<" "<<spherecoordinates[j*3+2]<<endl;
#endif
                    if (spherecoordinates[j*3] < BBox->Xmin)
                        BBox->Xmin = spherecoordinates[j*3];
                    if (spherecoordinates[j*3+1] < BBox->Ymin)
                        BBox->Ymin = spherecoordinates[j*3+1];
                    if (spherecoordinates[j*3+2] < BBox->Zmin)
                        BBox->Zmin = spherecoordinates[j*3+2];

                    if (spherecoordinates[j*3] < 30000 && spherecoordinates[j*3] > BBox->Xmax)
                        BBox->Xmax = spherecoordinates[j*3];
                    if (spherecoordinates[j*3+1] < 30000 && spherecoordinates[j*3+1] > BBox->Ymax)
                        BBox->Ymax = spherecoordinates[j*3+1];
                    if (spherecoordinates[j*3+2] < 30000 && spherecoordinates[j*3+2] > BBox->Zmax)
                        BBox->Zmax = spherecoordinates[j*3+2];

		    		    if(spherecoordinates[j*3] > 10000 || spherecoordinates[j*3+1] > 10000 || spherecoordinates[j*3+2] > 10000)
		    std::cout<<"Coord aberante("<<j<<") : ["<<spherecoordinates[j*3]<<","<<spherecoordinates[j*3+1]<<","<<spherecoordinates[j*3+2]<<"] "<<std::endl;

                }
		std::cout<<"Boite englobante : ("<<BBox->Xmin<<","<<BBox->Ymin<<","<<BBox->Zmin<<") ("<<BBox->Xmax<<","<<BBox->Ymax<<","<<BBox->Zmax<<")"<<std::endl;
            }


            else if(strcmp(var->name(),"sphereradii")==0)
            {
                sphereradii = (float *) var->values()->base();
                //cout<<"sphereradii : "<<endl;
            }

            else if(strcmp(var->name(),"spherescale")==0)

            {

                spherescale = (float *) var->values()->base();

            }


            else if(strcmp(var->name(),"spherecolors")==0)
            {
                spherecolors = (float *) var->values()->base();
                /*cout<<"spherecolors : "<<endl;
                for(unsigned j=0;j< sphere_number  ;j++)
                    {
                    cout <<spherecolors[j*4]<<" "<<spherecolors[j*4+1]<<" "<<spherecolors[j*4+2]<<" "<<spherecolors[j*4+3]<<endl;
                    }*/
            }


            else if(strcmp(var->name(),"cylinderids")==0)
            {
                cylinderids = (int *) var->values()->base();
                /*cout<<"cylinderids : "<<endl;
                for(unsigned j=0;j< cylinder_number  ;j++)
                    {
                    cout <<cylinderids[j]<<endl;
                    }*/
            }

            else if(strcmp(var->name(),"cylinderrefs")==0)
            {
                cylinderrefs = (int *) var->values()->base();
                //cout<<"cylinderrefs : "<<endl;
#ifdef VERBOSE
                for(unsigned j=0;j< cylinder_number  ;j++)
                {
                    cout<<cylinderrefs[j*2]<<" "<<cylinderrefs[j*2+1]<<endl;
                }
#endif
            }

            else if(strcmp(var->name(),"cylindershrink")==0)

            {

                cylindershrink = (float *) var->values()->base();
#ifdef VERBOSE
                cout<<"cylindershrink : "<<endl;
#endif
            }

            else if(strcmp(var->name(),"cylinderscale")==0)

            {

                cylinderscale = (float *) var->values()->base();
#ifdef VERBOSE
                cout<<"cylinderscale : "<<endl;
#endif
            }
            else if(strcmp(var->name(), "spherenames")==0)
            {
                spherenames = (char*) var->values()->base();
            }
        }

        setSpheresNumber(sphere_number);
        for(unsigned i = 0; i < sphere_number; i++)
        {
            _spheres[i].id=(unsigned)sphereids[i];

            _spheres[i].color.red = spherecolors[i*4];
            _spheres[i].color.green = spherecolors[i*4+1];
            _spheres[i].color.blue = spherecolors[i*4+2];
            _spheres[i].color.alpha = spherecolors[i*4+3];
            _spheres[i].radius = sphereradii[i];
	    //if(_spheres[i].id == 69952) _spheres[i].radius = 0.5;

            _spheres[i].scale = spherescale[i];

            char * name = &(spherenames[i*spherename_length]);

            QRegExp regex("(\\w)_(\\S{1,3})_(\\S{1,4})_(\\w{1,4})");

            regex.indexIn(QString(name));
	    //std::cout<<name<<"["<<_spheres[i].id<<"]"<<"("<<_spheres[i].radius<<";"<< _spheres[i].scale<<")"<<std::endl;
            
            _spheres[i].chain = regex.cap(1).at(0).toAscii();
            //std::cout<<"Nom de la chaine : "<<_spheres[i].chain<<std::endl;
            strncpy(_spheres[i].residue_name, regex.cap(2).toAscii().data(), 3);
            QRegExp alpha("\\D");
            if(regex.cap(3).contains(alpha)){
                QString hexa = "0x";
                hexa += regex.cap(3);
                cout << hexa.toStdString() << endl;
                _spheres[i].residue_id = hexa.toInt();
            }else{
                _spheres[i].residue_id = regex.cap(3).toInt();
            }
            strncpy(_spheres[i].name, regex.cap(4).toAscii().data(), 4);
        }

        setCylindersNumber(cylinder_number);
        for(unsigned i = 0; i < cylinder_number; i++)
        {
            _cylinders[i].id = (unsigned) cylinderids[i];

            _cylinders[i].idsrc = (unsigned) cylinderrefs[i*2];
            _cylinders[i].iddest = (unsigned) cylinderrefs[i*2+1];
            _cylinders[i].shrink = cylindershrink[i];

            _cylinders[i].scale = cylinderscale[i];

        }
        std::cout<<"Le fichier contient "<<sphere_number<<" atoms and "<<cylinder_number<<" liens."<<std::endl;
    }
    else
    {
        //cout<< "File " << _filename << " not found";
    }

    file->close();
}


unsigned NetCDFMolRepReader::getSpheresNumber() const
{
    return _spheresnum;
}
unsigned NetCDFMolRepReader::getCylindersNumber() const
{
    return _cylindersnum;
}

void NetCDFMolRepReader::setSpheresNumber(unsigned spheresnum)
{
    _spheresnum = spheresnum;
    if(_spheres!=0)
        free(_spheres);
    _spheres=(spheretype *) malloc (sizeof(spheretype)*_spheresnum);
}
void NetCDFMolRepReader::setCylindersNumber(unsigned cylindersnum)
{
    _cylindersnum=cylindersnum;
    if(_cylinders!=0)
        free(_cylinders);
    _cylinders=(cylindertype *) malloc (sizeof(cylindertype)*_cylindersnum);
}

void NetCDFMolRepReader::getCenter(float * center) const
{
    if(_spheresnum==0)
    {
        center[0]=0.0;
        center[1]=0.0;
        center[2]=0.0;
    }
    else
    {
        float maxX=spherecoordinates[0], minX=spherecoordinates[0], maxY=spherecoordinates[1], minY=spherecoordinates[1], maxZ=spherecoordinates[2], minZ=spherecoordinates[2];

        for(unsigned i=0;i<_spheresnum;i++)
        {
            if( spherecoordinates[i*3]<minX)
                minX =spherecoordinates[i*3];
            if(spherecoordinates[i*3]>maxX)
                maxX=spherecoordinates[i*3];
            if(spherecoordinates[i*3+1]<minY)
                minY = spherecoordinates[i*3+1];
            if(spherecoordinates[i*3+1]>maxY)
                maxY=spherecoordinates[i*3+1];

            if(spherecoordinates[i*3+2]<minZ)
                minZ =spherecoordinates[i*3+2];
            if(spherecoordinates[i*3+2]>maxZ)
                maxZ=spherecoordinates[i*3+2];
        }


        center[0]=(minX+maxX)/2.0;
        center[1]=(minY+maxY)/2.0;
        center[2]=(minZ+maxZ)/2.0;
    }
}


void NetCDFMolRepReader::getSpheres(spheretype * spheres) const
{
    memcpy(spheres, _spheres, sizeof(spheretype)*_spheresnum);
}
void NetCDFMolRepReader::getCylinders(cylindertype * cylinders) const
{
    memcpy(cylinders,_cylinders,sizeof(cylindertype)*_cylindersnum);
}

void NetCDFMolRepReader::getPositions(float * positions) const
{
    for(unsigned i=0;i<_spheresnum;i++)
    {
        positions[i*3]=spherecoordinates[i*3];
        positions[i*3+1]=spherecoordinates[i*3+1];
        positions[i*3+2]=spherecoordinates[i*3+2];
    }
}

void NetCDFMolRepReader::getBB(BBoxtype* BB_) const
{
    memcpy(BB_, BBox, sizeof(BBoxtype));
}

