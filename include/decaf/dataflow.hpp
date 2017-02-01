//---------------------------------------------------------------------------
// dataflow interface
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATAFLOW_HPP
#define DECAF_DATAFLOW_HPP

#include <map>

#include <decaf/data_model/pconstructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>

// transport layer specific types
#ifdef TRANSPORT_MPI
#include <decaf/transport/mpi/types.h>
#include <decaf/transport/mpi/comm.hpp>
#include <decaf/transport/mpi/redist_count_mpi.h>
#include <decaf/transport/mpi/redist_round_mpi.h>
#include <decaf/transport/mpi/redist_zcurve_mpi.h>
#include <decaf/transport/mpi/redist_block_mpi.h>
#include <decaf/transport/mpi/redist_proc_mpi.h>
#include <decaf/transport/mpi/tools.hpp>
#include <decaf/transport/mpi/channel.hpp>
#include <memory>
#include <queue>
#endif

#include "types.hpp"

namespace decaf
{
    // world is partitioned into {producer, dataflow, consumer, other} in increasing rank
    class Dataflow
    {
    public:
        Dataflow(CommHandle world_comm,          // world communicator
                 DecafSizes& decaf_sizes,        // sizes of producer, dataflow, consumer
                 int prod,                       // id in workflow structure of producer node
                 int dflow,                      // id in workflow structure of dataflow link
                 int con,                        // id in workflow structure of consumer node
                 Decomposition prod_dflow_redist // decompositon between producer and dataflow
                 = DECAF_CONTIG_DECOMP,
                 Decomposition dflow_cons_redist // decomposition between dataflow and consumer
                 = DECAF_CONTIG_DECOMP,
                 BufferMethod buffer_mode
                 = DECAF_BUFFER_NONE);
        ~Dataflow();
        void put(pConstructData data, TaskType role);
        void get(pConstructData data, TaskType role);
        DecafSizes* sizes()    { return &sizes_; }
        void shutdown();
        void err()             { ::all_err(err_); }
        // whether this rank is producer, dataflow, or consumer (may have multiple roles)
        bool is_prod()          { return((type_ & DECAF_PRODUCER_COMM) == DECAF_PRODUCER_COMM); }
        bool is_dflow()         { return((type_ & DECAF_DATAFLOW_COMM) == DECAF_DATAFLOW_COMM); }
        bool is_con()           { return((type_ & DECAF_CONSUMER_COMM) == DECAF_CONSUMER_COMM); }
        bool is_prod_root()     { return world_rank_ == sizes_.prod_start; }
        bool is_dflow_root()     { return world_rank_ == sizes_.dflow_start; }
        bool is_con_root()     { return world_rank_ == sizes_.con_start; }
        CommHandle prod_comm_handle() { return prod_comm_->handle(); }
        CommHandle con_comm_handle()  { return con_comm_->handle();  }
        Comm* prod_comm()             { return prod_comm_; }
        Comm* con_comm()              { return con_comm_;  }
        void forward();

        // Clear the buffer of the redistribution components.
        // To call if the data model change from one iteration to another or to free memory space
        void clearBuffers(TaskType role);

        // sets a quit message into a container; caller still needs to send the message
        static
        void
        set_quit(pConstructData out_data)   // output message
            {
                shared_ptr<SimpleConstructData<int> > data  =
                    make_shared<SimpleConstructData<int> >(1);
                out_data->appendData(string("decaf_quit"), data,
                                     DECAF_NOFLAG, DECAF_SYSTEM,
                                     DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
                out_data->setSystem(true);
            }

        // tests whether a message is a quit command
        static
        bool
        test_quit(pConstructData in_data)   // input message
        {
            return in_data->hasData(string("decaf_quit"));
        }

        void waitSignal(TaskType role);             // If a link and buffer activated, wait until we have the command to continue
        bool checkRootDflowSignal(TaskType role);   // If a link and buffer activated, check if we have the command to continue
        void signalRootDflowReady();                // If a cons and buffer activated, signal to the link that we are ready to receive a new message
        void broadcastDflowSignal();                // The dflow root broadcast a signal to the other dflow ranks to send the next msg
        bool checkDflowSignal();                    // Checking the sending signal in the dflow window
        void signalRootProdBlock(int value);        // Send the signal from the root dflow to producer to block/unblock the producer
        void broadcastProdSignal(int value);        // Broadcast a value from the producer root to the other producer ranks
        int getRootProdValue();                     // Return the command value from the dflow root to the producer root
        int getProdValue();                         // Return the command value from the prod root to all the producer


    private:
        CommHandle world_comm_;          // handle to original world communicator
        Comm* prod_comm_;                // producer communicator
        Comm* con_comm_;                 // consumer communicator
        Comm* dflow_comm_;               // dflow communicator
        int world_rank_;
        int world_size_;
        RedistComp* redist_prod_dflow_;  // Redistribution component between producer and dataflow
        RedistComp* redist_dflow_con_;   // Redestribution component between a dataflow and consumer
        RedistComp* redist_prod_con_;    // Redistribution component between producer and consumer
        DecafSizes sizes_;               // sizes of communicators, time steps
        int err_;                        // last error
        CommTypeDecaf type_;             // whether this instance is producer, consumer,
                                         // dataflow, or other
        int wflow_prod_id_;              // index of corresponding producer in the workflow
        int wflow_con_id_;               // index of corresponding consumer in the workflow
        int wflow_dflow_id_;             // index of corresponding link in the workflow
        bool no_link;                    // True if the Dataflow doesn't have a Link
        bool use_buffer;                 // True if the Dataflow manages a buffer.

        // Buffer infos
        BufferMethod bufferMethod_;         // Type of buffer to use
        int command_root_dflow_con_;        // Memory allocation for the root window between the consumer and the dflow
        MPI_Win window_root_dflow_con_;     // Window allocation for the root communications
        OneWayChannel* channel_dflow_con_;   // Communication channel between the root consumer and dflow
        int command_dflow_;                 // Memory allocation for the dflow window
        MPI_Win window_dflow_;              // Window allocation of the dflow communications
        int command_root_prod_dflow_;       // Memory allocation for the root window between the dflow and producer
        MPI_Win window_root_prod_dflow_;    // Window allocation of the root communications form dflow to the producer
        OneWayChannel* channel_prod_dflow_;  // Communication channel between the root dflow and prod
        int command_prod_;                  // Memory allocation of the prod window
        MPI_Win window_prod_;               // Window allocation of the prod communications
        OneWayChannel* channel_prod_;        //Communication channel between the producers
        bool first_iteration;
        std::queue<pConstructData> buffer;  // Buffer
        int buffer_max_size;                // Maximum size allowed for the buffer
        bool is_blocking;                   // Currently blocking the producer
        bool doGet;                         // We do a get until we get a terminate message
        CommHandle root_dc_comm_;           // Communicator including the root of dflow and root of cons
        int rank_dflow_root_dc_comm_;       // Rank of the dflow root in root_dc_comm_
        int rank_con_root_dc_comm_;         // Rank of the con root in root_dc_comm_
        CommHandle root_pd_comm_;           // Communicator including the root of dflow and root of cons
        int rank_dflow_root_pd_comm_;       // Rank of the dflow root in the root_pd_comm_
        int rank_prod_root_pd_comm_;        // Rank of the dflow root in the root_pd_comm_
    };

} // namespace

decaf::
Dataflow::Dataflow(CommHandle world_comm,
                   DecafSizes& decaf_sizes,
                   int prod,
                   int dflow,
                   int con,
                   Decomposition prod_dflow_redist,
                   Decomposition dflow_cons_redist,
                   BufferMethod buffer_mode) :
    world_comm_(world_comm),
    sizes_(decaf_sizes),
    wflow_prod_id_(prod),
    wflow_dflow_id_(dflow),
    wflow_con_id_(con),
    redist_prod_dflow_(NULL),
    redist_dflow_con_(NULL),
    redist_prod_con_(NULL),
    type_(DECAF_OTHER_COMM),
    no_link(false),
    use_buffer(false),
    buffer_max_size(2),
    is_blocking(false),
    first_iteration(true),
    doGet(true),
    root_dc_comm_(MPI_COMM_NULL),
    root_pd_comm_(MPI_COMM_NULL),
    bufferMethod_(buffer_mode)
{
    // DEPRECATED
    // sizes is a POD struct, initialization was not allowed in C++03; used assignment workaround
    // in C++11, apparently able to initialize as above by saying sizes_(decaf_sizes)
    // sizes_.prod_size = decaf_sizes.prod_size;
    // sizes_.dflow_size = decaf_sizes.dflow_size;
    // sizes_.con_size = decaf_sizes.con_size;
    // sizes_.prod_start = decaf_sizes.prod_start;
    // sizes_.dflow_start = decaf_sizes.dflow_start;
    // sizes_.con_start = decaf_sizes.con_start;
    // sizes_.con_nsteps = decaf_sizes.con_nsteps;

    // debug, confirming that above initialization works in all compilers
    assert(sizes_.prod_size == decaf_sizes.prod_size);
    assert(sizes_.dflow_size == decaf_sizes.dflow_size);
    assert(sizes_.con_size == decaf_sizes.con_size);
    assert(sizes_.prod_start == decaf_sizes.prod_start);
    assert(sizes_.dflow_start == decaf_sizes.dflow_start);
    assert(sizes_.con_start == decaf_sizes.con_start);

    world_rank_ = CommRank(world_comm); // my place in the world
    world_size_ = CommSize(world_comm);

    fprintf(stderr, "My rank: %i\n", world_rank_);

    // ensure sizes and starts fit in the world
    if (sizes_.prod_start + sizes_.prod_size > world_size_   ||
        sizes_.dflow_start + sizes_.dflow_size > world_size_ ||
        sizes_.con_start + sizes_.con_size > world_size_)
    {
        err_ = DECAF_COMM_SIZES_ERR;
        return;
    }

    // debug
    // if (world_rank == 0)
    //     fprintf(stderr, "prod: start %d size %d dflow: start %d size %d con: start %d size %d\n",
    //             sizes_.prod_start, sizes_.prod_size, sizes_.dflow_start, sizes_.dflow_size,
    //             sizes_.con_start, sizes_.con_size);

    // communicators

    if (world_rank_ >= sizes_.prod_start &&                   // producer
        world_rank_ < sizes_.prod_start + sizes_.prod_size)
    {
        type_ |= DECAF_PRODUCER_COMM;
        prod_comm_ = new Comm(world_comm, sizes_.prod_start, sizes_.prod_start + sizes_.prod_size - 1);
    }
    if (world_rank_ >= sizes_.dflow_start &&                  // dataflow
        world_rank_ < sizes_.dflow_start + sizes_.dflow_size)
    {
        type_ |= DECAF_DATAFLOW_COMM;
        dflow_comm_ = new Comm(world_comm, sizes_.dflow_start, sizes_.dflow_start + sizes_.dflow_size - 1);
    }
    if (world_rank_ >= sizes_.con_start &&                    // consumer
        world_rank_ < sizes_.con_start + sizes_.con_size)
    {
        type_ |= DECAF_CONSUMER_COMM;
        con_comm_ = new Comm(world_comm, sizes_.con_start, sizes_.con_start + sizes_.con_size - 1);
    }

    // producer and dataflow
    if (sizes_.dflow_size > 0 && (
        (world_rank_ >= sizes_.prod_start && world_rank_ < sizes_.prod_start + sizes_.prod_size) ||
        (world_rank_ >= sizes_.dflow_start && world_rank_ < sizes_.dflow_start + sizes_.dflow_size)))
    {
        switch(prod_dflow_redist)
        {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            // fprintf(stderr, "Using Round for prod -> dflow\n");
            redist_prod_dflow_ = new RedistRoundMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            // fprintf(stderr, "Using Count for prod -> dflow prod\n");
            redist_prod_dflow_ = new RedistCountMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            // fprintf(stderr, "Using ZCurve for prod -> dflow\n");
            redist_prod_dflow_ = new RedistZCurveMPI(sizes_.prod_start,
                                                     sizes_.prod_size,
                                                     sizes_.dflow_start,
                                                     sizes_.dflow_size,
                                                     world_comm);
            break;
        }
        case DECAF_BLOCK_DECOMP:
        {
            redist_prod_dflow_ = new RedistBlockMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }
        case DECAF_PROC_DECOMP:
        {
            redist_prod_dflow_ = new RedistProcMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   world_comm);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                    "Using RedistCountMPI instead\n", prod_dflow_redist);
            redist_prod_dflow_ = new RedistCountMPI(sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    world_comm);
            break;
        }

        }
    }
    else
    {
        //fprintf(stderr, "No redistribution between producer and dflow needed.\n");
        redist_prod_dflow_ = NULL;
    }

    // consumer and dataflow
    if (sizes_.dflow_size > 0 && (
        (world_rank_ >= sizes_.dflow_start && world_rank_ < sizes_.dflow_start + sizes_.dflow_size) ||
        (world_rank_ >= sizes_.con_start && world_rank_ < sizes_.con_start + sizes_.con_size)))
    {
        switch(dflow_cons_redist)
        {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            // fprintf(stderr, "Using Round for dflow -> cons\n");
            redist_dflow_con_ = new RedistRoundMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            // fprintf(stderr, "Using Count for dflow -> cons\n");
            redist_dflow_con_ = new RedistCountMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            // fprintf(stderr, "Using ZCurve for dflow -> cons\n");
            redist_dflow_con_ = new RedistZCurveMPI(sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    sizes_.con_start,
                                                    sizes_.con_size,
                                                    world_comm);
            break;
        }
        case DECAF_BLOCK_DECOMP:
        {
            redist_dflow_con_ = new RedistBlockMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_PROC_DECOMP:
        {

            redist_dflow_con_ = new RedistProcMPI(sizes_.dflow_start,
                                                  sizes_.dflow_size,
                                                  sizes_.con_start,
                                                  sizes_.con_size,
                                                  world_comm);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                    "Using RedistCountMPI instead\n", prod_dflow_redist);
            redist_dflow_con_ = new RedistCountMPI(sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }

        }
    }
    else
    {
        //fprintf(stderr, "No redistribution between dflow and cons needed.\n");
        redist_dflow_con_ = NULL;
    }

    // producer and consumer
    if (sizes_.dflow_size == 0 && (
        (world_rank_ >= sizes_.prod_start && world_rank_ < sizes_.prod_start + sizes_.prod_size) ||
        (world_rank_ >= sizes_.con_start && world_rank_ < sizes_.con_start + sizes_.con_size)))
    {
        no_link = true;

        switch(prod_dflow_redist)
        {
        case DECAF_ROUND_ROBIN_DECOMP:
        {
            // fprintf(stderr, "Using Round for dflow -> cons\n");
            redist_prod_con_ = new RedistRoundMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_CONTIG_DECOMP:
        {
            // fprintf(stderr, "Using Count for dflow -> cons\n");
            redist_prod_con_ = new RedistCountMPI(sizes_.prod_start,
                                                  sizes_.prod_size,
                                                  sizes_.con_start,
                                                  sizes_.con_size,
                                                  world_comm);
            break;
        }
        case DECAF_ZCURVE_DECOMP:
        {
            // fprintf(stderr, "Using ZCurve for dflow -> cons\n");
            redist_prod_con_ = new RedistZCurveMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }
        case DECAF_BLOCK_DECOMP:
        {
            redist_prod_con_ = new RedistBlockMPI(sizes_.prod_start,
                                                  sizes_.prod_size,
                                                  sizes_.con_start,
                                                  sizes_.con_size,
                                                  world_comm);
            break;
        }
        case DECAF_PROC_DECOMP:
        {

            redist_prod_con_ = new RedistProcMPI(sizes_.prod_start,
                                                 sizes_.prod_size,
                                                 sizes_.con_start,
                                                 sizes_.con_size,
                                                 world_comm);
            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                    "Using RedistCountMPI instead\n", prod_dflow_redist);
            redist_dflow_con_ = new RedistCountMPI(sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.con_start,
                                                   sizes_.con_size,
                                                   world_comm);
            break;
        }

        }
    }
    else
        redist_prod_con_ = NULL;

    if(buffer_mode != DECAF_BUFFER_NONE && !no_link)
        use_buffer = true;

    // Buffering setup
    if(!no_link && use_buffer)
    {
        // Creating the group of with the root link and root consumer
        if(is_con_root() || is_dflow_root())
        {
            /*
            MPI_Group group, groupRoots;
            MPI_Comm_group(world_comm, &group);

            int ranges[2][3];
            if(sizes_.dflow_start <= sizes_.con_start)
            {
                ranges[0][0] = sizes_.dflow_start;
                ranges[0][1] = sizes_.dflow_start;
                ranges[0][2] = 1;
                ranges[1][0] = sizes_.con_start;
                ranges[1][1] = sizes_.con_start;
                ranges[1][2] = 1;
                rank_dflow_root_dc_comm_ = 0;
                rank_con_root_dc_comm_ = 1;
            }
            else
            {
                ranges[1][0] = sizes_.dflow_start;
                ranges[1][1] = sizes_.dflow_start;
                ranges[1][2] = 1;
                ranges[0][0] = sizes_.con_start;
                ranges[0][1] = sizes_.con_start;
                ranges[0][2] = 1;
                rank_dflow_root_dc_comm_ = 1;
                rank_con_root_dc_comm_ = 0;
            }
            MPI_Group_range_incl(group, 2, ranges, &groupRoots);
            MPI_Comm_create_group(world_comm, groupRoots, 0, &root_dc_comm_);

            command_root_dflow_con_ = 1;    // 0 = sending msg, other = block

            // Only the root create a Window for the retroactive loop
            MPI_Win_create(&command_root_dflow_con_, 1, sizeof(int), MPI_INFO_NULL, root_dc_comm_, &window_root_dflow_con_);
            */
            channel_dflow_con_ = new OneWayChannel(world_comm_,
                                               sizes_.con_start,
                                               sizes_.dflow_start,
                                               1,
                                               (int)DECAF_CHANNEL_WAIT);
        }

        // Creating the group of with the root link and root consumer
        if(is_prod_root() || is_dflow_root())
        {

            /*
            MPI_Group group, groupRoots;
            MPI_Comm_group(world_comm, &group);

            int ranges[2][3];
            if(sizes_.dflow_start <= sizes_.prod_start)
            {
                ranges[0][0] = sizes_.dflow_start;
                ranges[0][1] = sizes_.dflow_start;
                ranges[0][2] = 1;
                ranges[1][0] = sizes_.prod_start;
                ranges[1][1] = sizes_.prod_start;
                ranges[1][2] = 1;
                rank_dflow_root_pd_comm_ = 0;
                rank_prod_root_pd_comm_ = 1;
            }
            else
            {
                ranges[1][0] = sizes_.dflow_start;
                ranges[1][1] = sizes_.dflow_start;
                ranges[1][2] = 1;
                ranges[0][0] = sizes_.prod_start;
                ranges[0][1] = sizes_.prod_start;
                ranges[0][2] = 1;
                rank_dflow_root_pd_comm_ = 1;
                rank_prod_root_pd_comm_ = 0;
            }
            MPI_Group_range_incl(group, 2, ranges, &groupRoots);
            MPI_Comm_create_group(world_comm, groupRoots, 0, &root_pd_comm_);

            command_root_prod_dflow_ = 0;    // 0 = sending msg, other = block

            // Only the root create a Window for the retroactive loop
            MPI_Win_create(&command_root_prod_dflow_, 1, sizeof(int), MPI_INFO_NULL, root_pd_comm_, &window_root_prod_dflow_);
            */
            channel_prod_dflow_ = new OneWayChannel(world_comm_,
                                                sizes_.dflow_start,
                                                sizes_.prod_start,
                                                1,
                                                (int)DECAF_CHANNEL_OK);
        }

        if(is_dflow())
        {
            command_dflow_ = 1;

            MPI_Win_create(&command_dflow_, 1, sizeof(int), MPI_INFO_NULL, dflow_comm_->handle(), &window_dflow_);
        }

        if(is_prod())
        {
            command_prod_ = 0;  // By default you are always allowed to send until you're blocked

            int err = MPI_Win_create(&command_prod_, 1, sizeof(int), MPI_INFO_NULL, prod_comm_handle(), &window_prod_);
            fprintf(stderr, "ERROR on the window: %i\n", err);
            fprintf(stderr,"Creation of the prod channel...\n");
            channel_prod_ = new OneWayChannel(world_comm_,
                                          sizes_.prod_start,
                                          sizes_.prod_start,
                                          sizes_.prod_size,
                                          (int)DECAF_CHANNEL_OK);
            fprintf(stderr,"Prod channel created\n");
        }
    }

    err_ = DECAF_OK;
}

decaf::
Dataflow::~Dataflow()
{
    if (is_prod())
        delete prod_comm_;
    if (is_con())
        delete con_comm_;
    if (redist_dflow_con_)
        // TODO: Following crashes on my mac, for the hacc example, in rank 8 (dflow)
        // but not on other machines so far. Will see if it happens in other contexts,
        // or if it is just my outdated software stack -- TP
        delete redist_dflow_con_;
    if (redist_prod_dflow_)
        delete redist_prod_dflow_;

    if(!no_link && use_buffer)
    {
        if(world_rank_ == sizes_.con_start || world_rank_ == sizes_.dflow_start)
            MPI_Win_free(&window_root_dflow_con_);
        if(is_dflow())
            MPI_Win_free(&window_dflow_);
    }
}

// puts data into the dataflow
// tags the data with the:
// - source type (producer/consumer or dataflow)
// - workflow link id corresponding to this dataflow
// - destination workflow node id or link id, depending on type of destination
// NB: source is *link* id while destination is *either node or link* id
void
decaf::
Dataflow::put(pConstructData data, TaskType role)
{

    // clear old identifiers
    data->removeData("src_type");
    data->removeData("link_id");
    data->removeData("dest_id");

    // encode type into message (producer/consumer or dataflow)
    shared_ptr<SimpleConstructData<TaskType> > value  =
        make_shared<SimpleConstructData<TaskType> >(role);
    data->appendData(string("src_type"), value,
                    DECAF_NOFLAG, DECAF_SYSTEM,
                    DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


    // encode dataflow link id into message
    shared_ptr<SimpleConstructData<int> > value1  =
        make_shared<SimpleConstructData<int> >(wflow_dflow_id_);
    data->appendData(string("link_id"), value1,
                     DECAF_NOFLAG, DECAF_SYSTEM,
                     DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


    if (role == DECAF_NODE)
    {
        if(no_link)
        {
            // encode destination link id into message
            shared_ptr<SimpleConstructData<int> > value2  =
                make_shared<SimpleConstructData<int> >(wflow_con_id_);
            data->appendData(string("dest_id"), value2,
                             DECAF_NOFLAG, DECAF_SYSTEM,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

            // send the message
            if(redist_prod_con_ == NULL)
                fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
            redist_prod_con_->process(data, DECAF_REDIST_SOURCE);
            redist_prod_con_->flush();
        }
        else
        {
            if(use_buffer)
            {
                bool blocking = false;
                if(is_prod_root())
                {
                    //int command_received = getRootProdValue();
                    //if(command_received == 1)
                    DecafChannelCommand command_received;
                    if(first_iteration)
                    {
                        command_received = DECAF_CHANNEL_OK;
                        first_iteration = false;
                    }
                    else
                        command_received = channel_prod_dflow_->checkSelfCommand();
                    if(command_received == DECAF_CHANNEL_WAIT)
                    {
                        fprintf(stderr,"Blocking command received. We should block\n");
                        fprintf(stderr,"Broadcasting on prod root the value 1\n");

                        //broadcastProdSignal(1);
                        channel_prod_->sendCommand(DECAF_CHANNEL_WAIT);
                        blocking = true;
                    }
                }
                MPI_Barrier(prod_comm_handle());

                // TODO: remove busy wait
                //int signal_prod;
                DecafChannelCommand signal_prod;
                do
                {
                    fprintf(stderr,"Checking value from the produer\n");
                    //signal_prod = getProdValue();
                    signal_prod = channel_prod_->checkSelfCommand();

                    if(is_prod_root() && blocking == true)
                    {
                        //int command_received = getRootProdValue();
                        DecafChannelCommand command_received = channel_prod_dflow_->checkSelfCommand();
                        //if(command_received == 0)
                        if(command_received == DECAF_CHANNEL_OK)
                        {
                            fprintf(stderr,"Unblocking command received.\n");
                            fprintf(stderr,"Broadcasting on prod root the value 0\n");

                            //broadcastProdSignal(0);
                            channel_prod_->sendCommand(DECAF_CHANNEL_OK);
                            break;
                        }
                    }

                    usleep(100000); //100ms


                 //} while(signal_prod != 0);
                 } while(signal_prod != DECAF_CHANNEL_OK);
            }

            // encode destination link id into message
            shared_ptr<SimpleConstructData<int> > value2  =
                make_shared<SimpleConstructData<int> >(wflow_dflow_id_);
            data->appendData(string("dest_id"), value2,
                             DECAF_NOFLAG, DECAF_SYSTEM,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

            // send the message
            if(redist_prod_dflow_ == NULL)
                fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
            redist_prod_dflow_->process(data, DECAF_REDIST_SOURCE);
            redist_prod_dflow_->flush();
        }
    }
    else if (role == DECAF_LINK)
    {
        // encode destination node id into message
        shared_ptr<SimpleConstructData<int> > value2  =
            make_shared<SimpleConstructData<int> >(wflow_con_id_);
        data->appendData(string("dest_id"), value2,
                         DECAF_NOFLAG, DECAF_SYSTEM,
                         DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


        // send the message
        if(redist_dflow_con_ == NULL)
            fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
        redist_dflow_con_->process(data, DECAF_REDIST_SOURCE);
        redist_dflow_con_->flush();
    }

    data->removeData("src_type");
    data->removeData("link_id");
    data->removeData("dest_id");
}

// Not used anymore
void
decaf::
Dataflow::waitSignal(TaskType role)
{
    // Just for links for now
    if(role != DECAF_LINK)
        return;

    // First iteration we send immediatly to start the pipeline
    if(first_iteration)
    {
        first_iteration = false;
        return;
    }

    // Check the window
    int localCommand = 1;
    do
    {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_dflow_root_dc_comm_, 0, window_root_dflow_con_);
        MPI_Get(&localCommand, 1, MPI_INT, rank_dflow_root_dc_comm_, 0, 1, MPI_INT, window_root_dflow_con_);
        if(localCommand == 0)   // We are good to go
        {
            int newFlag = 1;
            MPI_Put(&newFlag, 1, MPI_INT, rank_dflow_root_dc_comm_, 0, 1, MPI_INT, window_root_dflow_con_);
        }
        MPI_Win_unlock(rank_dflow_root_dc_comm_, window_root_dflow_con_);
    } while(localCommand != 0);
}

// The root consumer sent a signal to the root dflow
void
decaf::
Dataflow::signalRootDflowReady()
{
    if(!use_buffer || !is_con_root())
        return;

    // Only for consumer for now
    //MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_dflow_root_dc_comm_, 0, window_root_dflow_con_);
    //int flag_to_send = 0;
    //MPI_Put(&flag_to_send, 1, MPI_INT, rank_dflow_root_dc_comm_, 0, 1, MPI_INT, window_root_dflow_con_);
    //MPI_Win_unlock(rank_dflow_root_dc_comm_, window_root_dflow_con_);
    fprintf(stderr,"Signaling the dflow that the con is ready\n");
    channel_dflow_con_->sendCommand(DECAF_CHANNEL_OK);
}

// Check in the root dflow if we received a signal from the con
bool
decaf::
Dataflow::checkRootDflowSignal(TaskType role)
{
    // Just for links for now
    if(!is_dflow_root())
        return false;

    // First iteration we send immediatly to start the pipeline
    if(first_iteration)
    {
        first_iteration = false;
        return true;
    }

    // Checking the window to see if the command has been updated
    // 0 = send
    // other = block
    int localCommand = 1;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_dflow_root_dc_comm_, 0, window_root_dflow_con_);
    MPI_Get(&localCommand, 1, MPI_INT, rank_dflow_root_dc_comm_, 0, 1, MPI_INT, window_root_dflow_con_);
    if(localCommand == 0)   // We are good to go
    {
        // Reseting the window value for next iteration
        int newFlag = 1;
        MPI_Put(&newFlag, 1, MPI_INT, rank_dflow_root_dc_comm_, 0, 1, MPI_INT, window_root_dflow_con_);
        MPI_Win_unlock(rank_dflow_root_dc_comm_, window_root_dflow_con_);

        return true;
    }

    // We didn't have the command
    MPI_Win_unlock(rank_dflow_root_dc_comm_, window_root_dflow_con_);

    return false;
}

// The root consumer sent a signal to the root dflow
void
decaf::
Dataflow::signalRootProdBlock(int value)
{
    if(!use_buffer || !is_dflow_root())
        return;

    // Only for consumer for now
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_prod_root_pd_comm_, 0, window_root_prod_dflow_);
    int flag_to_send = value;
    MPI_Put(&flag_to_send, 1, MPI_INT, rank_prod_root_pd_comm_, 0, 1, MPI_INT, window_root_prod_dflow_);
    MPI_Win_unlock(rank_prod_root_pd_comm_, window_root_prod_dflow_);
}

// Check in the root dflow if we received a signal from the con
int
decaf::
Dataflow::getProdValue()
{
    // Just for links for now
    if(!is_prod_root())
        return 0;

    // First iteration we send immediatly to start the pipeline
    if(first_iteration)
    {
        first_iteration = false;
        return 0;
    }

    // Checking the window to see if the command has been updated
    // 0 = send
    // other = block
    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, prod_comm_->rank(), 0, window_prod_);
    MPI_Get(&localCommand, 1, MPI_INT, prod_comm_->rank(), 0, 1, MPI_INT, window_prod_);

    // We didn't have the command
    MPI_Win_unlock(prod_comm_->rank(), window_prod_);

    return localCommand;
}

void
decaf::
Dataflow::broadcastDflowSignal()
{
    if(!is_dflow_root())
        return;

    for(int i = 0; i < dflow_comm_->size(); i++)
    {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, i, 0, window_dflow_);
        int flag_to_send = 0;
        MPI_Put(&flag_to_send, 1, MPI_INT, i, 0, 1, MPI_INT, window_dflow_);
        MPI_Win_unlock(i, window_dflow_);
    }
}

void
decaf::
Dataflow::broadcastProdSignal(int value)
{
    if(!is_prod_root())
        return;

    for(int i = 0; i < prod_comm_->size(); i++)
    {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, i, 0, window_prod_);
        int flag_to_send = value;
        MPI_Put(&flag_to_send, 1, MPI_INT, i, 0, 1, MPI_INT, window_prod_);
        MPI_Win_unlock(i, window_prod_);
    }
}

int
decaf::
Dataflow::getRootProdValue()
{
    if(!is_prod())
        return 0;

    // First iteration we send immediatly to start the pipeline
    if(first_iteration)
    {
        first_iteration = false;
        return 0;
    }

    // Checking the window to see if the command has been updated
    // 0 = send
    // other = block
    int localCommand = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_prod_root_pd_comm_, 0, window_root_prod_dflow_);
    MPI_Get(&localCommand, 1, MPI_INT, rank_prod_root_pd_comm_, 0, 1, MPI_INT, window_root_prod_dflow_);

    // We didn't have the command
    MPI_Win_unlock(rank_prod_root_pd_comm_, window_root_prod_dflow_);

    return localCommand;
}

bool
decaf::
Dataflow::checkDflowSignal()
{
    if(!is_dflow())
        return false;

    //fprintf(stderr, "Checking dflow signal to the rank %i\n", dflow_comm_->rank());
    // Checking the window to see if the command has been updated
    // 0 = send
    // other = block
    int localCommand = 1;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, dflow_comm_->rank(), 0, window_dflow_);
    MPI_Get(&localCommand, 1, MPI_INT, dflow_comm_->rank(), 0, 1, MPI_INT, window_dflow_);
    if(localCommand == 0)   // We are good to go
    {
        // Reseting the window value for next iteration
        int newFlag = 1;
        MPI_Put(&newFlag, 1, MPI_INT, dflow_comm_->rank(), 0, 1, MPI_INT, window_dflow_);
        MPI_Win_unlock(dflow_comm_->rank(), window_dflow_);

        return true;
    }

    // We didn't have the command
    MPI_Win_unlock(dflow_comm_->rank(), window_dflow_);

    return false;
}



void
decaf::
Dataflow::get(pConstructData data, TaskType role)
{
    if (role == DECAF_LINK)
    {
        if (redist_prod_dflow_ == NULL)
            fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");

        if(use_buffer)
        {
            bool receivedDflowSignal = false;

            // First phase: Waiting for the signal and checking incoming msgs
            while(!receivedDflowSignal)
            {
                // Checking the root communication
                if(is_dflow_root())
                {
                    bool receivedRootSignal;
                    if(first_iteration)
                    {
                        first_iteration = false;
                        receivedRootSignal =  true;
                    }
                    else
                        receivedRootSignal =  channel_dflow_con_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT);

                    //bool receivedRootSignal = checkRootDflowSignal(role);
                    if(receivedRootSignal)
                    {
                        broadcastDflowSignal();
                        fprintf(stderr,"Dflow received the continue command. Broadcasting\n");
                    }

                }

                // NOTE: Barrier could be removed if we used only async point-to-point communication
                // For now collective are creating deadlocks without barrier
                // Ex: 1 dflow do put while the other do a get
                MPI_Barrier(dflow_comm_->handle());

                receivedDflowSignal = checkDflowSignal();
                //receivedDflowSignal = channel_dflow_con_.checkSelfCommand() == DECAF_CHANNEL_OK;

                if(doGet && !is_blocking)
                {
                    pConstructData container;
                    bool received = redist_prod_dflow_->IGet(container);
                    if(received)
                    {
                        if(Dataflow::test_quit(container))
                            doGet = false;
                        redist_prod_dflow_->flush();
                        buffer.push(container);

                        // Block the producer if reaching the maximum buffer size
                        if(buffer.size() >= buffer_max_size && !is_blocking)
                        {
                            fprintf(stderr,"The buffer is too large. Sending blocking command\n");
                            is_blocking = true;
                            //signalRootProdBlock(1);
                            if(is_dflow_root())
                                channel_prod_dflow_->sendCommand(DECAF_CHANNEL_WAIT);
                        }
                    }

                    fprintf(stderr, "Size of the buffer: %u\n", buffer.size());
                }
            }

            fprintf(stderr, "The signal from the consumer was received\n");

            // Second phase: We got the signal
            if(buffer.empty()) // No data received yet, we do a blocking get
            {
                if(!doGet)
                {
                    fprintf(stderr, "ERROR: trying to get after receiving a terminate msg.\n");
                }
                // We have no msg in queue, we treat directly the incoming msg
                redist_prod_dflow_->process(data, DECAF_REDIST_DEST);
                redist_prod_dflow_->flush();
            }
            else
            {
                // Won't do a copy, just pass the pointer of the map
                data->merge(buffer.front().getPtr());
                buffer.pop();

                // Unblock the producer if necessary
                if(buffer.size() < buffer_max_size && is_blocking)
                {
                    fprintf(stderr,"The buffer is small enough. Sending unblocking command\n");
                    is_blocking = false;
                    //signalRootProdBlock(0);
                    if(is_dflow_root())
                        channel_prod_dflow_->sendCommand(DECAF_CHANNEL_OK);
                }
            }
        }
        else
        {
            redist_prod_dflow_->process(data, DECAF_REDIST_DEST);
            redist_prod_dflow_->flush();
        }
    }
    else if (role == DECAF_NODE)
    {
        if(no_link)
        {
            if (redist_prod_con_ == NULL)
                fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
            redist_prod_con_->process(data, DECAF_REDIST_DEST);
            redist_prod_con_->flush();
        }
        else
        {
            if (redist_dflow_con_ == NULL)
                fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
            redist_dflow_con_->process(data, DECAF_REDIST_DEST);
            redist_dflow_con_->flush();
        }
    }
}

// cleanup
void
decaf::
Dataflow::shutdown()
{
    if (redist_prod_dflow_)
        redist_prod_dflow_->shutdown();
    if (redist_dflow_con_)
        redist_dflow_con_->shutdown();
    if(redist_prod_con_)
        redist_prod_con_->shutdown();
}


void
decaf::
Dataflow::clearBuffers(TaskType role)
{
    if (role == DECAF_LINK)
        redist_dflow_con_->clearBuffers();
    else if (role == DECAF_NODE && no_link)
        redist_prod_con_->clearBuffers();
    else if (role == DECAF_NODE && !no_link)
        redist_prod_dflow_->clearBuffers();
}

#endif
