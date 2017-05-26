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
* File: put.cpp                                                   *
*                                                                 *
* Contacts:                                                       *
*  26/02/2004 Jeremie Allard <Jeremie.Allard@imag.fr>             *
*                                                                 *
******************************************************************/
#include <flowvr/module.h>
#include <iostream>
#include <unistd.h>

//Decaf
#include <bredala/data_model/simpleconstructdata.hpp>
#include <bredala/data_model/constructtype.h>
#include <bredala/data_model/arrayconstructdata.hpp>
#include <bredala/data_model/baseconstructdata.hpp>
#include <decaf/flowvr/flowvr_wrap.hpp>


#define ARRAY_SIZE 100

using namespace decaf;
using namespace std;

int sleep_time=2;

int main(int argc, const char** argv)
{
  flowvr::OutputPort pOut("text");
  std::vector<flowvr::Port*> ports;
  ports.push_back(&pOut);
  flowvr::ModuleAPI* flowvr = flowvr::initModule(ports);
  if (flowvr == NULL)
  {
    return 1;
  }

  int it=0;
  while (flowvr->wait())
  {
    flowvr::MessageWrite m;

    int *array = new int[ARRAY_SIZE];
    for(int i = 0; i < ARRAY_SIZE; i++)
        array[i] = i;

    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    std::shared_ptr<SimpleConstructData<int> > itData =
            std::make_shared<SimpleConstructData<int> >( it, container->getMap() );
    container->appendData("it", itData,
                          DECAF_NOFLAG, DECAF_SHARED,
                          DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

    std::shared_ptr<ArrayConstructData<int> > arrayData =
            std::make_shared<ArrayConstructData<int> >(
                array, ARRAY_SIZE, 1, false, container->getMap());
    container->appendData("array", arrayData,
                          DECAF_NOFLAG, DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

    put(flowvr, m, &pOut, container);

    sleep(sleep_time);
    ++it;
  }

  flowvr->close();

  return 0;
}
