#ifndef FLOWVR_WRAP_HPP
#define FLOWVR_WRAP_HPP

#include <decaf/data_model/constructtype.h>
#include <flowvr/module.h>

using namespace decaf;

bool put(flowvr::ModuleAPI* module,
         flowvr::MessageWrite& msg,
         flowvr::OutputPort* pOut,
         std::shared_ptr<ConstructData> container)
{
    if(container->serialize())
    {
        msg.data = module->alloc(container->getOutSerialBufferSize());
        memcpy(msg.data.writeAccess(), container->getOutSerialBuffer(), container->getOutSerialBufferSize());
        module->put(pOut, msg);
    }
    else
    {
        std::cerr<<"ERROR : unable to serialize the data."<<std::endl;
        return false;
    }

    return true;
}

std::shared_ptr<ConstructData> get(flowvr::ModuleAPI* module,
                                   flowvr::InputPort* pIn)
{
    flowvr::Message m;
    module->get(pIn,m);

    std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
    container->allocate_serial_buffer(m.data.getSize());
    memcpy(container->getInSerialBuffer(), m.data.readAccess(), m.data.getSize());
    container->merge();

    return container;
}



#endif
