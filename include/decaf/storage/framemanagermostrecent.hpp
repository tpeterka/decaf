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

#ifndef DECAF_FRAMEMANAGER_RECENT
#define DECAF_FRAMEMANAGER_RECENT

#include <decaf/storage/framemanager.hpp>
#include <list>

namespace decaf
{

    class FrameManagerRecent : public FrameManager
    {
    public:
        FrameManagerRecent(CommHandle comm);
        virtual ~FrameManagerRecent();

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
    };
}

decaf::
FrameManagerRecent::FrameManagerRecent(CommHandle comm): FrameManager(comm),
    received_frame_(false), received_frame_id_(false), previous_frame_(-1)
{
    channel_->updateSelfValue(-1);
}

decaf::
FrameManagerRecent::~FrameManagerRecent()
{

}

void
decaf::
FrameManagerRecent::putFrame(unsigned int id)
{
    buffer_.push_back(id);
}

// Check if we have received the frame ID from the master
bool
decaf::
FrameManagerRecent::hasNextFrameId()
{
    if(!received_frame_id_)
    {
        int new_frame_id = channel_->checkSelfInt();
        if(new_frame_id != previous_frame_)
        {
            received_frame_id_ = true;
            frame_to_send_ = new_frame_id;
        }
    }

    return received_frame_id_;
}

// Check if we have receive the actual frame we are supposed to send
bool
decaf::
FrameManagerRecent::hasNextFrame()
{
    // We don't even know what frame to send yet
    if(!hasNextFrameId())
        return false;

    // checking in the buffer if we have the desired frame
    for(auto& it : buffer_)
    {
        if(it == frame_to_send_)
            return true;
    }

    return false;
}

// Take the most recent frame id received and broadcast it to the rest of the dflows
void
decaf::
FrameManagerRecent::computeNextFrame()
{
    // Only the master should compute the next frame
    if(!channel_->is_source())
        return;

    int next_frame;
    // We have not received a new frame yet
    // We will send the next one
    if(buffer_.empty())
        next_frame = previous_frame_ + 1;
    else
        next_frame = buffer_.back(); // The most recent frame is at the back

    // Sending the frame to the other dflows
    channel_->sendInt(next_frame);
}

FrameCommand
decaf::
FrameManagerRecent::getNextFrame(unsigned int* frame_id)
{
    *frame_id = frame_to_send_;

    // Reseting the parameters
    previous_frame_ = frame_to_send_;
    received_frame_ = false;
    received_frame_id_ = false;

    while(buffer_.front() != frame_to_send_)
        buffer_.pop_front();

    // We will remove all frames before the frame sent
    return DECAF_FRAME_REMOVE_UNTIL;
}

#endif
