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
#include "transport/mpi/types.h"
#include "transport/mpi/comm.hpp"
#include "transport/mpi/redist_count_mpi.h"
#include "transport/mpi/redist_round_mpi.h"
#include "transport/mpi/redist_zcurve_mpi.h"
#include "transport/mpi/redist_block_mpi.h"
#include "transport/mpi/redist_proc_mpi.h"
#include "transport/mpi/tools.hpp"
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
                 = DECAF_CONTIG_DECOMP);
        ~Dataflow();
        void put(pConstructData data, TaskType role);
        void get(pConstructData data, TaskType role);
        DecafSizes* sizes()    { return &sizes_; }
        void shutdown();
        void err()             { ::all_err(err_); }
        // whether this rank is producer, dataflow, or consumer (may have multiple roles)
        bool is_prod()         { return((type_ & DECAF_PRODUCER_COMM) == DECAF_PRODUCER_COMM); }
        bool is_dflow()        { return((type_ & DECAF_DATAFLOW_COMM) == DECAF_DATAFLOW_COMM); }
        bool is_con()          { return((type_ & DECAF_CONSUMER_COMM) == DECAF_CONSUMER_COMM); }
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
        bool checkSignal(TaskType role);            // If a link and buffer activated, check if we have the command to continue
        void signalReady();            // If a cons and buffer activated, signal to the link that we are ready to receive a new message


    private:
        CommHandle world_comm_;          // handle to original world communicator
        Comm* prod_comm_;                // producer communicator
        Comm* con_comm_;                 // consumer communicator
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
        int command;
        MPI_Win window;
        bool first_iteration;
        std::queue<pConstructData> buffer;  // Buffer
        bool doGet;                         // We do a get until we get a terminate message
        CommHandle root_comm_;              // Communicator including the root of dflow and root of cons
        int rank_dflow_root_comm_;          // Rank of the dflow root in root_comm_
        int rank_con_root_comm_;            // Rank of the con root in root_comm_
        };

} // namespace

decaf::
Dataflow::Dataflow(CommHandle world_comm,
                   DecafSizes& decaf_sizes,
                   int prod,
                   int dflow,
                   int con,
                   Decomposition prod_dflow_redist,
                   Decomposition dflow_cons_redist) :
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
    use_buffer(true),
    first_iteration(true),
    doGet(true),
    root_comm_(MPI_COMM_NULL)
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

    int world_rank = CommRank(world_comm); // my place in the world
    int world_size = CommSize(world_comm);

    // ensure sizes and starts fit in the world
    if (sizes_.prod_start + sizes_.prod_size > world_size   ||
        sizes_.dflow_start + sizes_.dflow_size > world_size ||
        sizes_.con_start + sizes_.con_size > world_size)
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

    if (world_rank >= sizes_.prod_start &&                   // producer
        world_rank < sizes_.prod_start + sizes_.prod_size)
    {
        type_ |= DECAF_PRODUCER_COMM;
        prod_comm_ = new Comm(world_comm, sizes_.prod_start, sizes_.prod_start + sizes_.prod_size - 1);
    }
    if (world_rank >= sizes_.dflow_start &&                  // dataflow
        world_rank < sizes_.dflow_start + sizes_.dflow_size)
        type_ |= DECAF_DATAFLOW_COMM;
    if (world_rank >= sizes_.con_start &&                    // consumer
        world_rank < sizes_.con_start + sizes_.con_size)
    {
        type_ |= DECAF_CONSUMER_COMM;
        con_comm_ = new Comm(world_comm, sizes_.con_start, sizes_.con_start + sizes_.con_size - 1);
    }

    // producer and dataflow
    if (sizes_.dflow_size > 0 && (
        (world_rank >= sizes_.prod_start && world_rank < sizes_.prod_start + sizes_.prod_size) ||
        (world_rank >= sizes_.dflow_start && world_rank < sizes_.dflow_start + sizes_.dflow_size)))
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
        (world_rank >= sizes_.dflow_start && world_rank < sizes_.dflow_start + sizes_.dflow_size) ||
        (world_rank >= sizes_.con_start && world_rank < sizes_.con_start + sizes_.con_size)))
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
        (world_rank >= sizes_.prod_start && world_rank < sizes_.prod_start + sizes_.prod_size) ||
        (world_rank >= sizes_.con_start && world_rank < sizes_.con_start + sizes_.con_size)))
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

    // Buffering setup
    if(!no_link && use_buffer)
    {
        // Creating the group of with the root link and root consumer
        if(world_rank == sizes_.con_start || world_rank == sizes_.dflow_start)
        {
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
                rank_dflow_root_comm_ = 0;
                rank_con_root_comm_ = 1;
            }
            else
            {
                ranges[1][0] = sizes_.dflow_start;
                ranges[1][1] = sizes_.dflow_start;
                ranges[1][2] = 1;
                ranges[0][0] = sizes_.con_start;
                ranges[0][1] = sizes_.con_start;
                ranges[0][2] = 1;
                rank_dflow_root_comm_ = 1;
                rank_con_root_comm_ = 0;
            }
            MPI_Group_range_incl(group, 2, ranges, &groupRoots);
            MPI_Comm_create_group(world_comm, groupRoots, 0, &root_comm_);


            // Only the root create a Window for the retroactive loop
            MPI_Win_create(&command, 1, sizeof(int), MPI_INFO_NULL, root_comm_, &window);
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

    if(use_buffer)
        MPI_Win_free(&window);
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
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_dflow_root_comm_, 0, window);
        MPI_Get(&localCommand, 1, MPI_INT, rank_dflow_root_comm_, 0, 1, MPI_INT, window);
        if(localCommand == 0)   // We are good to go
        {
            int newFlag = 1;
            MPI_Put(&newFlag, 1, MPI_INT, rank_dflow_root_comm_, 0, 1, MPI_INT, window);
        }
        MPI_Win_unlock(rank_dflow_root_comm_, window);
    } while(localCommand != 0);

    fprintf(stderr, "We received the signal. Processing...\n");
}

bool
decaf::
Dataflow::checkSignal(TaskType role)
{
    // Just for links for now
    if(role != DECAF_LINK)
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
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_dflow_root_comm_, 0, window);
    MPI_Get(&localCommand, 1, MPI_INT, rank_dflow_root_comm_, 0, 1, MPI_INT, window);
    if(localCommand == 0)   // We are good to go
    {
        // Reseting the window value for next iteration
        int newFlag = 1;
        MPI_Put(&newFlag, 1, MPI_INT, rank_dflow_root_comm_, 0, 1, MPI_INT, window);
        MPI_Win_unlock(rank_dflow_root_comm_, window);

        return true;
    }

    // We didn't have the command
    MPI_Win_unlock(rank_dflow_root_comm_, window);

    return false;
}

void
decaf::
Dataflow::signalReady()
{
    if(!is_con())
        return;

    // Only for consumer for now
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank_dflow_root_comm_, 0, window);
    int flag_to_send = 0;
    MPI_Put(&flag_to_send, 1, MPI_INT, rank_dflow_root_comm_, 0, 1, MPI_INT, window);
    MPI_Win_unlock(rank_dflow_root_comm_, window);
    fprintf(stderr, "Sent the signal to the dflow.\n");
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
            bool receivedSignal = false;

            // First phase: Waiting for the signal and checking incoming msgs
            while(!receivedSignal)
            {
                receivedSignal = checkSignal(role);

                if(doGet)
                {
                    pConstructData container;
                    bool received = redist_prod_dflow_->IGet(container);
                    if(received)
                    {
                        if(Dataflow::test_quit(container))
                        {
                            doGet = false;
                            fprintf(stderr, "Termination received. Won't perform a get anymore.\n");
                        }
                        redist_prod_dflow_->flush();
                        buffer.push(container);
                        fprintf(stderr, "New size of the queue: %zu\n", buffer.size());
                    }
                }

            }

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
                fprintf(stderr, "Dequeing a msg. New queue size: %zu\n", buffer.size());
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
        fprintf(stderr, "Waiting for data in con...\n");
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
        fprintf(stderr, "Data received in con.\n");
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
