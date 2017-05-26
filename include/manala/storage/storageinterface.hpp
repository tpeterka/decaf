//---------------------------------------------------------------------------
// Define the interface of a storage object to store frames
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_STORAGE_INTERFACE
#define DECAF_STORAGE_INTERFACE

#include <bredala/data_model/pconstructtype.h>
#include <manala/types.h>

namespace decaf
{
    class Storage
    {
    public:
        Storage(){}

        virtual ~Storage(){}

        virtual bool isFull() = 0;
        virtual unsigned int getBufferSize() = 0;
        virtual bool insert(unsigned int id, pConstructData data) = 0;
        virtual void erase(unsigned int id) = 0;
        virtual bool hasData(unsigned int id) = 0;
        virtual pConstructData getData(unsigned int id) = 0;
        virtual void processCommand(FrameCommand command, unsigned int frame_id) = 0;
        virtual unsigned int getID(unsigned int index) = 0;
        virtual unsigned int getNbDataStored() = 0;
    };
} // namespace decaf

#endif
