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

#ifndef DECAF_FRAMEMANAGER_LOWHIGH
#define DECAF_FRAMEMANAGER_LOWHIGH

#include <manala/selector/framemanager.hpp>
#include <list>

namespace decaf
{

    class FrameManagerLowHigh : public FrameManager
    {
    public:
        FrameManagerLowHigh(CommHandle comm, TaskType role, unsigned int low_freq_output, unsigned int high_freq_output);
        virtual ~FrameManagerLowHigh();

        virtual bool sendFrame(unsigned int id);
        virtual void putFrame(unsigned int id);
        virtual FrameCommand getNextFrame(unsigned int* frame_id);
        virtual bool hasNextFrameId();
        virtual bool hasNextFrame();
        virtual void computeNextFrame();

    protected:
        std::list<unsigned int> buffer_;
        bool received_frame_id_;
        bool received_frame_;
        int frame_to_send_;
        int previous_frame_;
        unsigned int low_freq_output_;   // Output frequency guarantee by the selector
        unsigned int high_freq_output_;  // Output frequency from the producer
    };
}

#endif
