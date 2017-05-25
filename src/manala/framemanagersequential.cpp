#include <manala/selector/framemanagersequential.hpp>

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
