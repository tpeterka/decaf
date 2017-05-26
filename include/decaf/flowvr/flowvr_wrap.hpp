#ifndef FLOWVR_WRAP_HPP
#define FLOWVR_WRAP_HPP

#include <bredala/data_model/constructtype.h>
#include <flowvr/module.h>

using namespace decaf;

bool containerToMsg(flowvr::ModuleAPI* module,
                    flowvr::MessageWrite& msg,
                    std::shared_ptr<ConstructData> container)
{
    if(container->serialize())
    {
        msg.data = module->alloc(container->getOutSerialBufferSize());
        memcpy(msg.data.writeAccess(), container->getOutSerialBuffer(), container->getOutSerialBufferSize());
    }
    else
    {
        std::cerr<<"ERROR : unable to serialize the data."<<std::endl;
        return false;
    }

    return true;
}

bool put(flowvr::ModuleAPI* module,
         flowvr::MessageWrite& msg,
         flowvr::OutputPort* pOut,
         std::shared_ptr<ConstructData> container)
{
    if(containerToMsg(module, msg, container))
    {
        module->put(pOut, msg);
        return true;
    }
    else
        return false;
}

std::shared_ptr<ConstructData> get(flowvr::ModuleAPI* module,
                                   flowvr::Message& msg,
                                   flowvr::InputPort* pIn)
{
    module->get(pIn,msg);

    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    container->allocate_serial_buffer(msg.data.getSize());
    memcpy(container->getInSerialBuffer(), msg.data.readAccess(), msg.data.getSize());
    container->merge();

    return container;
}



#endif
