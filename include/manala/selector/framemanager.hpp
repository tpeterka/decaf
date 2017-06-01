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

#include <decaf/types.hpp>
#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/channel.hpp>
#include <manala/types.h>
#include <mpi.h>

namespace decaf
{
    class FrameManager
    {
    public:
        FrameManager(CommHandle comm, TaskType role);
        virtual ~FrameManager();

        // Prod side
        // Return true if the producer should send the frame id
        virtual bool sendFrame(unsigned int id) = 0;

        // Link side
        // Notify the FrameManager of the arrival of a new frame
        virtual void putFrame(unsigned int id) = 0;

        // Link side
        // Perform computations to decide the frame ID of the next frame to send
        // Can involve communications to synchronize
        virtual void computeNextFrame() = 0;

        // Link side
        // Check if we have received the frame ID from the master
        virtual bool hasNextFrameId() = 0;

        // Link side
        // Check if we have receive the actual frame we are supposed to send
        virtual bool hasNextFrame() = 0;

        // Link side
        // Return the frame ID to send next and a command for the storage collection.
        // WARNING: should not be called before hasNextFrameId() return true.
        virtual FrameCommand getNextFrame(unsigned int* frame_id) = 0;

        // Return true if the local process is rank 0 of link
        bool isMaster(){ return master_; }

    protected:
        bool master_;               // Rank 0 in the link
        int comm_rank_;             // Rank in the link communicator
        int comm_size_;             // Size of the link communicator
        CommHandle comm_;           // Communicator of the link
        OneWayChannel* channel_;    // Communication channel to broadcast commands
        bool first_iteration_;      // True is first iteration
        unsigned int previous_id_;  // Last frame id sent
        TaskType role_;             // Producer or link
        unsigned int prod_sample;   // Output frequency for the producer
    };

}
#endif
