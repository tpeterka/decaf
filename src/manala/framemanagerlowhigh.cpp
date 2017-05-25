#include <manala/selector/framemanagerlowhigh.hpp>

decaf::
FrameManagerLowHigh::FrameManagerLowHigh(CommHandle comm, TaskType role, unsigned int low_freq_output, unsigned int high_freq_output):
    FrameManager(comm, role),
    received_frame_(false), received_frame_id_(false), previous_frame_(-1),
    low_freq_output_(low_freq_output), high_freq_output_(high_freq_output)
{

    if(low_freq_output_ <= high_freq_output_)
    {
        fprintf(stderr, "ERROR: low frequency higher or equal than high frequency. Should be lower.\n");
        exit(1);
    }
    // We don't have a communicator for the producer side.
    if(role == DECAF_LINK)
        channel_->updateSelfValue(-1);
}

decaf::
FrameManagerLowHigh::~FrameManagerLowHigh()
{

}

// On the producer side, we output at high frequency
bool
decaf::
FrameManagerLowHigh::sendFrame(unsigned int id)
{
    return id % high_freq_output_ == 0;
}

void
decaf::
FrameManagerLowHigh::putFrame(unsigned int id)
{
    buffer_.push_back(id);
}

// Check if we have received the frame ID from the master
bool
decaf::
FrameManagerLowHigh::hasNextFrameId()
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
FrameManagerLowHigh::hasNextFrame()
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

// Nothing to do
void
decaf::
FrameManagerLowHigh::computeNextFrame()
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
    {
        // We are looking in the old frames
        // if there is a low freq output
        if(buffer_.back() / low_freq_output_ == previous_frame_ / low_freq_output_)
        {
            //The most recent frame is still in the same low freauency period
            next_frame = buffer_.back();
        }
        else // Finding the oldest low_freq
        {
            // Going through the buffer until we reach a low freq
            for(auto it : buffer_)
            {
                if(it % low_freq_output_ == 0)
                {
                    next_frame = it;
                    break;
                }
            }
        }
    }

    // Sending the frame to the other dflows
    channel_->sendInt(next_frame);
}

FrameCommand
decaf::
FrameManagerLowHigh::getNextFrame(unsigned int* frame_id)
{
    *frame_id = frame_to_send_;

    // Reseting the parameters
    previous_frame_ = frame_to_send_;
    received_frame_ = false;
    received_frame_id_ = false;

    while(buffer_.front() != frame_to_send_)
        buffer_.pop_front();
    buffer_.pop_front(); // We remove also the frame equal to frame_to_send_

    // We will remove all frames before the frame sent
    return DECAF_FRAME_COMMAND_REMOVE_UNTIL;
}
