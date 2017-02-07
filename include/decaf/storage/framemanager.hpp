//---------------------------------------------------------------------------
// Framemanager interface. This interface provide a way to implement a
// sampling policy on incomming data streams
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------
#ifndef DECAF_FRAMEMANAGER
#define DECAF_FRAMEMANAGER

#include <decaf/transport/mpi/types.h>
#include <decaf/transport/mpi/channel.hpp>
#include <mpi.h>

enum FrameCommand
{
    FRAME_REMOVE,
    FRAME_REMOVE_UNTIL,
    FRAME_REMOVE_UNTIL_EXCLUDED
};

namespace decaf
{
    class FrameManager
    {
    public:
        FrameManager(CommHandle comm);
        virtual ~FrameManager();

        virtual void putFrame(unsigned int id) = 0;
        virtual FrameCommand getNextFrame(unsigned int* frame_id) = 0;
        virtual bool hasNextFrameId() = 0;
        virtual bool hasNextFrame() = 0;
        virtual void computeNextFrame() = 0;
        bool isMaster(){ return master_; }

    protected:
        bool master_;
        int comm_rank_;
        int comm_size_;
        CommHandle comm_;
        OneWayChannel* channel_;
        bool first_iteration_;
        unsigned int previous_id_;
    };

}

decaf::
FrameManager::FrameManager(CommHandle comm): comm_(comm), first_iteration_(true), channel_(NULL)
{
    MPI_Comm_rank(comm_, &comm_rank_);
    MPI_Comm_size(comm_, &comm_size_);

    master_ = comm_rank_ == 0;

    channel_ = new OneWayChannel(comm_, 0, 0, comm_size_, 0);
}

decaf::
FrameManager::~FrameManager()
{
    if(channel_) delete channel_;
}


#endif
