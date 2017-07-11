#include <bredala/transport/mpi/channel.hpp>
#include <cstdio>

decaf::
OneWayChannel::OneWayChannel() : initialized_(false){}

decaf::
OneWayChannel::OneWayChannel(   CommHandle world_comm,
                                int prod,
                                int startRecep,
                                int nbRecep,
                                int defaultMemoryValue)
{
    world_comm_ = world_comm;
    MPI_Comm_size(world_comm_, &world_size_);
    MPI_Comm_rank(world_comm_, &world_rank_);
    memory_ = defaultMemoryValue;

    if(world_rank_ != prod && (world_rank_ < startRecep || world_rank_ >= startRecep + nbRecep))
    {
        initialized_ = false;
        return;
    }

    // Creating the MPI communicator and Window with only the correct processes
    MPI_Group world_group, channel_group;
    MPI_Comm_group(world_comm_, &world_group);

    if(prod < startRecep)
    {
        int ranges[2][3];
        ranges[0][0] = prod;
        ranges[0][1] = prod;
        ranges[0][2] = 1;
        ranges[1][0] = startRecep;
        ranges[1][1] = startRecep + nbRecep - 1; // Max rank included
        ranges[1][2] = 1;
        rank_source_ = 0;
        rank_start_recep_ = 1;
        MPI_Group_range_incl(world_group, 2, ranges, &channel_group);
    }
    else if(prod >=  startRecep + nbRecep)
    {
        int ranges[2][3];
        ranges[1][0] = prod;
        ranges[1][1] = prod;
        ranges[1][2] = 1;
        ranges[0][0] = startRecep;
        ranges[0][1] = startRecep + nbRecep - 1;
        ranges[0][2] = 1;
        rank_source_ = nbRecep;
        rank_start_recep_ = 0;
        MPI_Group_range_incl(world_group, 2, ranges, &channel_group);
    }
    else // Producer is overlapping with receivers
    {
        int ranges[3];
        ranges[0] = startRecep;
        ranges[1] = startRecep + nbRecep - 1;
        ranges[2] = 1;
        rank_source_ = prod - startRecep;
        rank_start_recep_ = 0;
        MPI_Group_range_incl(world_group, 1, &ranges, &channel_group);
    }
    nb_recep_ = nbRecep;

    MPI_Comm_create_group(world_comm_, channel_group, 0, &channel_comm_);

    MPI_Comm_rank(channel_comm_, &channel_rank_);
    MPI_Comm_size(channel_comm_, &channel_size_);

    MPI_Win_create(&memory_, 1, sizeof(int), MPI_INFO_NULL, channel_comm_, &window_);

    initialized_ = true;
}


// TODO: cause segfaults
decaf::
OneWayChannel::~OneWayChannel()
{
    if(initialized_)
    {
        MPI_Win_free(&window_);
        MPI_Comm_free(&channel_comm_);
    }
}

int
decaf::
OneWayChannel::channel_rank()
{
    if(!initialized_)
        return -1;
    return channel_rank_;
}

int
decaf::
OneWayChannel::channel_comm_size()
{
    if(!initialized_)
        return -1;
    return channel_size_;
}

bool
decaf::
OneWayChannel::is_source()
{
    return initialized_ && channel_rank_ == rank_source_;
}

bool
decaf::
OneWayChannel::is_recep()
{
    return  initialized_ &&
            channel_rank_ >= rank_start_recep_ &&
            channel_rank_ < rank_start_recep_ + nb_recep_;
}

bool
decaf::
OneWayChannel::is_in_channel()
{
    return initialized_;
}

void
decaf::
OneWayChannel::sendCommand(DecafChannelCommand command)
{
    if(!initialized_)
    {
        fprintf(stderr,"ERROR: the ChannelCommand is not initialized.\n");
        return;

    }

    for(int i = rank_start_recep_; i < rank_start_recep_ + nb_recep_; i++)
    {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, i, 0, window_);
        int flag_to_send = (int)command;
        MPI_Put(&flag_to_send, 1, MPI_INT, i, 0, 1, MPI_INT, window_);
        MPI_Win_unlock(i, window_);
    }
}
DecafChannelCommand
decaf::
OneWayChannel::checkSelfCommand()
{
    if(!initialized_)
        return DECAF_CHANNEL_OK;

    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, channel_rank_, 0, window_);
    MPI_Get(&localCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);

    // We didn't have the command
    MPI_Win_unlock(channel_rank_, window_);

    return (DecafChannelCommand)localCommand;
}

bool
decaf::
OneWayChannel::checkAndReplaceSelfCommand(
        DecafChannelCommand wanted_command,
        DecafChannelCommand replace_command)
{
    if(!initialized_)
        return false;

    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, channel_rank_, 0, window_);
    MPI_Get(&localCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
    if(localCommand == (int)wanted_command)
    {
        int newCommand = (int)replace_command;
        MPI_Put(&newCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
        MPI_Win_unlock(channel_rank_, window_);
        return true;
    }

    // We didn't have the command
    MPI_Win_unlock(channel_rank_, window_);

    return false;
}

void
decaf::
OneWayChannel::sendInt(int value)
{
    if(!initialized_)
    {
        fprintf(stderr,"ERROR: the ChannelCommand is not initialized.\n");
        return;

    }

    for(int i = rank_start_recep_; i < rank_start_recep_ + nb_recep_; i++)
    {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, i, 0, window_);
        int flag_to_send = value;
        MPI_Put(&flag_to_send, 1, MPI_INT, i, 0, 1, MPI_INT, window_);
        MPI_Win_unlock(i, window_);
    }
}

void
decaf::
OneWayChannel::updateSelfValue(int value)
{
    if(!initialized_)
    {
        fprintf(stderr,"ERROR: the ChannelCommand is not initialized.\n");
        return;

    }


    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, channel_rank_, 0, window_);
    int flag_to_send = value;
    MPI_Put(&flag_to_send, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
    MPI_Win_unlock(channel_rank_, window_);
}

bool
decaf::
OneWayChannel::checkAndReplaceSelfInt(int wanted_value,
                            int replace_value)
{
    if(!initialized_)
        return false;

    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, channel_rank_, 0, window_);
    MPI_Get(&localCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
    if(localCommand == wanted_value)
    {
        int newCommand = replace_value;
        MPI_Put(&newCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
        MPI_Win_unlock(channel_rank_, window_);
        return true;
    }

    // We didn't have the command
    MPI_Win_unlock(channel_rank_, window_);

    return false;
}

bool
decaf::
OneWayChannel::checkDifferentAndReplaceSelfInt(int different_value,
                                     int replace_value, int* received_value)
{
    if(!initialized_)
        return false;

    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, channel_rank_, 0, window_);
    MPI_Get(&localCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
    if(localCommand != different_value)
    {
        int newCommand = replace_value;
        MPI_Put(&newCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);
        MPI_Win_unlock(channel_rank_, window_);
        *received_value = localCommand;
        return true;
    }

    // We didn't have the command
    MPI_Win_unlock(channel_rank_, window_);

    return false;
}

int
decaf::
OneWayChannel::checkSelfInt()
{
    if(!initialized_)
        return DECAF_CHANNEL_OK;

    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, channel_rank_, 0, window_);
    MPI_Get(&localCommand, 1, MPI_INT, channel_rank_, 0, 1, MPI_INT, window_);

    // We didn't have the command
    MPI_Win_unlock(channel_rank_, window_);

    return localCommand;
}

