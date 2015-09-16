/******* COPYRIGHT ************************************************
*                                                                 *
*                             FlowVR                              *
*                     Application Library                         *
*                                                                 *
*-----------------------------------------------------------------*
 * COPYRIGHT (C) 2003-2011                by                       *
* INRIA                                                           *
* ALL RIGHTS RESERVED.	                                          *
*                                                                 *
* This source is covered by the GNU LGPL, please refer to the     *
* COPYING file for further information.                           *
*                                                                 *
*-----------------------------------------------------------------*
*                                                                 *
*  Original Contributors:                                         *
*    Jeremie Allard,                                              *
*    Thomas Arcila,                                               *
*    Jean-Denis Lesage.                                           *	
*    Clement Menier,                                              *
*    Bruno Raffin                                                 *
*                                                                 *
*******************************************************************
*                                                                 *
* File: get.cpp                                                   *
*                                                                 *
* Contacts:                                                       *
*  26/02/2004 Jeremie Allard <Jeremie.Allard@imag.fr>             *
*                                                                 *
******************************************************************/
#include "flowvr/module.h"
#include <iostream>
#include <unistd.h>

//Decaf
#include <decaf/data_model/simpleconstructdata.hpp>
#include <decaf/data_model/vectorconstructdata.hpp>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/arrayconstructdata.hpp>
#include <decaf/data_model/baseconstructdata.hpp>

using namespace decaf;
using namespace std;

int sleep_time=1;

int main(int argc, const char** argv)
{
  flowvr::InputPort pIn("text");
  std::vector<flowvr::Port*> ports;
  ports.push_back(&pIn);

  flowvr::ModuleAPI* flowvr = flowvr::initModule(ports);
  if (flowvr == NULL)
  {
    return 1;
  }

  int it=0;
  while (flowvr->wait())
  {
    // Get Message
    flowvr::Message m;
    flowvr->get(&pIn,m);

    /*// Read data
    std::string text;
    text.append((const char*)m.data.readAccess(),((const char*)m.data.readAccess())+m.data.getSize());

    // Log info
    std::string source;
    int mit;
    m.stamps.read(pIn.stamps->it,mit);
    m.stamps.read(pIn.stamps->source,source);
    std::cout<<"Received "<<text<<"(it="<<mit<<") from "<<source<<std::endl;*/

    //Extracting the datamodel from the message
    char* serialBuffer = (char*)(m.data.readAccess());
    int sizeSerialBuffer = m.data.getSize();
    std::cerr<<"Size of the serial buffer : "<<sizeSerialBuffer<<std::endl;
    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    container->allocate_serial_buffer(sizeSerialBuffer);
    memcpy(container->getInSerialBuffer(), serialBuffer, sizeSerialBuffer);
    container->merge();
    std::cerr<<"Unserialize ok"<<std::endl;

    //Extracting the fields
    std::shared_ptr<BaseConstructData> baseItData = container->getData("it");
    if(!baseItData)
    {
        std::cerr<<"ERROR : Could not find the field \"pos\" in the datamodel. Abording."<<std::endl;
        exit(1);
    }
    std::shared_ptr<SimpleConstructData<int> > itData =
            dynamic_pointer_cast<SimpleConstructData<int> >(baseItData);

    std::cout<<"Received : "<<itData->getData()<<std::endl;

    sleep(sleep_time);
    ++it;
  }

  flowvr->close();
  return 0;
}
