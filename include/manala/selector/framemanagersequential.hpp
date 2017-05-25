//---------------------------------------------------------------------------
// Framemanager sequential implementation. The frame are sent one by one
// in the receiving order.
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_FRAMEMANAGER_SEQ
#define DECAF_FRAMEMANAGER_SEQ

#include <manala/selector/framemanager.hpp>
#include <queue>

namespace decaf
{

    class FrameManagerSeq : public FrameManager
    {
    public:
        FrameManagerSeq(CommHandle comm, TaskType role, unsigned int prod_freq_output);
        virtual ~FrameManagerSeq();

        virtual bool sendFrame(unsigned int id);
        virtual void putFrame(unsigned int id);
        virtual FrameCommand getNextFrame(unsigned int* frame_id);
        virtual bool hasNextFrameId();
        virtual bool hasNextFrame();
        virtual void computeNextFrame();

    protected:
        std::queue<unsigned int> buffer_;
        unsigned int prod_freq_output_;     // Output frequency at the producer
    };
}

#endif
