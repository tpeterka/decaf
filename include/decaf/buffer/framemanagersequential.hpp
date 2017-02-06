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

#include <decaf/buffer/framemanager.hpp>
#include <queue>

namespace decaf
{

    class FrameManagerSeq : public FrameManager
    {
    public:
        FrameManagerSeq(CommHandle comm);
        virtual ~FrameManagerSeq();

        virtual void putFrame(unsigned int id);
        virtual FrameCommand getNextFrame(unsigned int* frame_id);
        virtual bool hasNextFrameId();
        virtual bool hasNextFrame();
        virtual void computeNextFrame();

    protected:
        std::queue<unsigned int> buffer_;
    };
}

decaf::
FrameManagerSeq::FrameManagerSeq(CommHandle comm): FrameManager(comm)
{

}

decaf::
FrameManagerSeq::~FrameManagerSeq()
{

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

    return FRAME_REMOVE;
}

#endif
