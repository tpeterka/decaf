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
#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/constructtype.h>
#include <bredala/data_model/arrayconstructdata.hpp>
#include <bredala/data_model/baseconstructdata.hpp>
#include <decaf/flowvr/flowvr_wrap.hpp>

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
    //Read the message and unserialize it
    flowvr::Message msg;
    std::shared_ptr<ConstructData> container = get(flowvr, msg, &pIn);

    //Extracting the fields
    std::shared_ptr<SimpleConstructData<int> > itData =
            container->getTypedData<SimpleConstructData<int> >("it");
    std::shared_ptr<ArrayConstructData<int> > arrayData =
            container->getTypedData<ArrayConstructData<int> >("array");

    int* array = arrayData->getArray();


    std::cout<<"Received : "<<itData->getData()<<std::endl;
    std::cout<<"[";
    for(unsigned int i = 0; i < 10; i++)
        std::cout<<array[i]<<" ";
    std::cout<<"]"<<std::endl;

    sleep(sleep_time);
    ++it;
  }

  flowvr->close();
  return 0;
}
