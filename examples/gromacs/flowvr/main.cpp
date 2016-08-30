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


#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include <QtCore/QDateTime>

#include <flowvr/module.h>
//#include <flowvr/render/primitive.h>

#include "vitamins_data.h"

#include "NetCDFMolRepReader.h"

int main(int argc,char ** argv)
{
    NetCDFMolRepReader * netcdfreader = new NetCDFMolRepReader();

    std::string filename;
    filename += std::string(argv[1]);

    netcdfreader->setFileName(filename.c_str());
    netcdfreader->read();
    unsigned nbspheres = netcdfreader->getSpheresNumber();
    unsigned nbcylinders = netcdfreader->getCylindersNumber();


    spheretype * spheres = (spheretype *) malloc (sizeof(spheretype)*nbspheres);
    cylindertype * cylinders=(cylindertype *) malloc (sizeof(cylindertype)*nbcylinders);
    float * center = (float *) malloc(sizeof(float)*3);
    float *positions = (float *) malloc(sizeof(float)*3*nbspheres);
    BBoxtype * Bounding = (BBoxtype*) malloc(sizeof(BBoxtype));

    netcdfreader->getBB(Bounding);
    netcdfreader->getCenter(center);
    netcdfreader->getSpheres(spheres);
    netcdfreader->getCylinders(cylinders);



    netcdfreader->getPositions(positions);

    flowvr::OutputPort* poutSphere = NULL;
    flowvr::OutputPort* poutCylinder = NULL;
    flowvr::OutputPort* poutCenter = NULL;
    flowvr::OutputPort* poutPosition = NULL;
    flowvr::OutputPort* poutBBox = NULL;

    // Declare a FlowVR output port
    poutSphere = new flowvr::OutputPort("outSphere");
    poutCylinder = new flowvr::OutputPort("outCylinder");
    poutCenter = new flowvr::OutputPort("outCenter");
    poutPosition = new flowvr::OutputPort("outPosition");
    poutBBox = new flowvr::OutputPort("outBBox");

    // Initialize FlowVR
    std::vector<flowvr::Port*> ports;
    ports.push_back(poutSphere);
    ports.push_back(poutCylinder);
    ports.push_back(poutCenter);
    ports.push_back(poutPosition);
    ports.push_back(poutBBox);

    flowvr::ModuleAPI* module = flowvr::initModule(ports);
    if (module == NULL)
        return 1;

    flowvr::MessageWrite m;
    flowvr::MessageWrite n;
    flowvr::MessageWrite o;
    flowvr::MessageWrite p;
    flowvr::MessageWrite q;

    m.data = module->alloc(nbspheres*sizeof(spheretype));
    n.data = module->alloc(nbcylinders*sizeof(cylindertype));
    o.data = module->alloc(3*sizeof(float));
    p.data = module->alloc(3*nbspheres*sizeof(float));
    q.data = module->alloc(sizeof(BBoxtype));

    spheretype* initial_spheres = (spheretype*) m.data.writeAccess();
    cylindertype* initial_cylinders = (cylindertype*) n.data.writeAccess();
    float* initial_center = (float*) o.data.writeAccess();
    float * initial_positions = (float*) p.data.writeAccess();
    BBoxtype* BoundingBox = (BBoxtype*) q.data.writeAccess();

    memcpy((char*) initial_spheres, (char*) spheres, nbspheres * sizeof(spheretype));
    memcpy((char*) initial_cylinders, (char*) cylinders, nbcylinders * sizeof(cylindertype));
    memcpy((char*) initial_center, (char*) center, 3 * sizeof(float));
    memcpy((char*) initial_positions, (char*) positions, 3 *nbspheres* sizeof(float));
    memcpy((char*) BoundingBox, (char*)Bounding, sizeof(BBoxtype));

    // Send message
    QDateTime start = QDateTime::currentDateTime();
    module->put(poutSphere, m);

    module->put(poutCylinder,n);
    module->put(poutCenter, o);
    module->put(poutBBox, q);

    std::cout << "ma bounding box : x=" << Bounding->Xmin << " y=" << Bounding->Ymin << " z=" << Bounding->Zmin << std::endl;
    std::cout << "ma bounding box : x=" << Bounding->Xmax << " y=" << Bounding->Ymax << " z=" << Bounding->Zmax << std::endl;
    /*std::cout << "message envoye center: " <<  std::endl;
    std::cout << "Centre("<<center[0] << ", "<< center[1] <<", "<<center[2] <<std::endl;*/

    module->put(poutPosition,p);
    // Main FlowVR loop.
    if(module->wait())
    {
        // Send message
        // scene.put(&pOut);
        start = QDateTime::currentDateTime();
        module->put(poutSphere, m);

        module->put(poutCylinder,n);
        module->put(poutCenter, o);
        //module->put(poutPosition,p);
        module->put(poutBBox, q);
        //std::cout << "dans la boucle : " <<  std::endl;
    }
    module->close();
    return 0;
}
