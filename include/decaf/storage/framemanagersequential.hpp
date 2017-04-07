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

#include <decaf/storage/framemanager.hpp>
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

decaf::
FrameManagerSeq::FrameManagerSeq(CommHandle comm, TaskType role, unsigned int prod_freq_output):
    FrameManager(comm, role), prod_freq_output_(prod_freq_output)
{

}

decaf::
FrameManagerSeq::~FrameManagerSeq()
{

}


bool
decaf::
FrameManagerSeq::sendFrame(unsigned int id)
{
    return id % prod_freq_output_ == 0;
}

void
decaf::
FrameManagerSeq::putFrame(unsigned int id)
{
    buffer_.push(id);
}

// Check if we have received the frame ID from the master
bool
decaf::
FrameManagerSeq::hasNextFrameId()
{
    return buffer_.size() > 0;
}

// Check if we have receive the actual frame we are supposed to send
bool
decaf::
FrameManagerSeq::hasNextFrame()
{
    return buffer_.size() > 0;
}

// Nothing to do
void
decaf::
FrameManagerSeq::computeNextFrame()
{
    return;
}

FrameCommand
decaf::
FrameManagerSeq::getNextFrame(unsigned int* frame_id)
{
    if(buffer_.empty())
    {
        fprintf(stderr,"ERROR: asking for next frame with no frame available.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    unsigned int frame = buffer_.front();
    buffer_.pop();

    *frame_id = frame;

    return DECAF_FRAME_COMMAND_REMOVE;
}

#endif
