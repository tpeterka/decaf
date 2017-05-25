#include <manala/selector/framemanager.hpp>

decaf::
FrameManager::FrameManager(CommHandle comm, TaskType role):
    comm_(comm), first_iteration_(true), channel_(NULL), role_(role)
{
    if(role == DECAF_LINK)
    {
        MPI_Comm_rank(comm_, &comm_rank_);
        MPI_Comm_size(comm_, &comm_size_);

        master_ = comm_rank_ == 0;

        channel_ = new OneWayChannel(comm_, 0, 0, comm_size_, 0);
    }
    else
        master_ = false;
}

decaf::
FrameManager::~FrameManager()
{
    if(channel_) delete channel_;
}
