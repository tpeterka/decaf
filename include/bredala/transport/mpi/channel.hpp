//---------------------------------------------------------------------------
// Interface to wrap one sided communication channels to send commands
// A channel can send a command from one producer to 1 or multiple senders
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#ifndef DECAF_CHANNEL_COMMAND
#define DECAF_CHANNEL_COMMAND

#ifdef TRANSPORT_MPI
#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/comm.h>
#endif

enum DecafChannelCommand
{
    DECAF_CHANNEL_OK = 0x0,
    DECAF_CHANNEL_WAIT = 0x1,
    DECAF_CHANNEL_GET = 0x2,
    DECAF_CHANNEL_PUT = 0x4
};

namespace decaf
{
    class OneWayChannel
    {
    public:
        OneWayChannel(  );
        OneWayChannel(  CommHandle world_comm,
                        int prod,
                        int startRecep,
                        int nbRecep,
                        int defaultMemoryValue);

        ~OneWayChannel();

        int channel_rank();
        int channel_comm_size();
        bool is_source();
        bool is_recep();
        bool is_in_channel();

        void sendCommand(DecafChannelCommand command);
        DecafChannelCommand checkSelfCommand();
        bool checkAndReplaceSelfCommand(DecafChannelCommand wanted_command,
                                        DecafChannelCommand replace_command);
        void sendInt(int value);
        void updateSelfValue(int value);
        bool checkAndReplaceSelfInt(int wanted_value,
                                    int replace_value);
        bool checkDifferentAndReplaceSelfInt(int different_value,
                                             int replace_value,
                                             int* received_value);

        int checkSelfInt();


    private:
        CommHandle world_comm_;
        CommHandle channel_comm_;
        int world_rank_;
        int channel_rank_;
        int world_size_;
        int channel_size_;

        int rank_source_;       // Rank of the source in the channel communicator
        int rank_start_recep_;  // Starting rank of the source in the channel communicator
        int nb_recep_;

        int memory_;            // Memory space to share the command
        MPI_Win window_;        // Window of the channel
        bool initialized_;      // True if the local MPI rank is belongs to the channel
    };
}

#endif
