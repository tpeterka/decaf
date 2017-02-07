//---------------------------------------------------------------------------
// Implement a double feedback mechanism with buffering
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#ifndef DECAF_DATASTREAM_DOUBLE_FEEDBACK
#define DECAF_DATASTREAM_DOUBLE_FEEDBACK

#include <decaf/datastream/datastreaminterface.hpp>
#include <decaf/transport/mpi/channel.hpp>
#include <decaf/transport/redist_comp.h>
#include <decaf/data_model/msgtool.hpp>
#include <decaf/storage/storagemainmemory.hpp>
#include <queue>

namespace decaf
{

    class DatastreamDoubleFeedback : public Datastream
    {
    public:
        DatastreamDoubleFeedback() : Datastream(){}
        DatastreamDoubleFeedback(CommHandle world_comm,
               int start_prod,
               int nb_prod,
               int start_dflow,
               int nb_dflow,
               int start_con,
               int nb_con,
               RedistComp* prod_dflow,
               RedistComp* dflow_con);

        virtual ~DatastreamDoubleFeedback();

        virtual void processProd(pConstructData data);

        virtual void processDflow(pConstructData data);

        virtual void processCon(pConstructData data);

    protected:
        unsigned int iteration_;
        unsigned int buffer_max_size_;
        Storage* buffer_;
        OneWayChannel* channel_prod_;
        OneWayChannel* channel_prod_dflow_;
        OneWayChannel* channel_dflow_;
        OneWayChannel* channel_dflow_con_;
        OneWayChannel* channel_con_;
        bool first_iteration_;
        bool doGet_;
        bool is_blocking_;
    };

decaf::
DatastreamDoubleFeedback::~DatastreamDoubleFeedback()
{
    if(channel_prod_) delete channel_prod_;
    if(channel_prod_dflow_) delete channel_prod_dflow_;
    if(channel_dflow_) delete channel_dflow_;
    if(channel_dflow_con_) delete channel_dflow_con_;
    if(channel_con_) delete channel_con_;
}

decaf::
DatastreamDoubleFeedback::DatastreamDoubleFeedback(CommHandle world_comm,
       int start_prod,
       int nb_prod,
       int start_dflow,
       int nb_dflow,
       int start_con,
       int nb_con,
       RedistComp* prod_dflow,
       RedistComp* dflow_con):
    Datastream(world_comm, start_prod, nb_prod, start_dflow, nb_dflow, start_con, nb_con, prod_dflow, dflow_con),
    channel_prod_(NULL), channel_prod_dflow_(NULL), channel_dflow_(NULL), channel_dflow_con_(NULL),channel_con_(NULL),
    first_iteration_(true), doGet_(true), is_blocking_(false), buffer_max_size_(5), iteration_(0),
    buffer_(NULL)
{
    initialized_ = true;
    // Creation of the channels
    if(is_prod())
    {
        channel_prod_ = new OneWayChannel(  world_comm_,
                                        start_prod,
                                        start_prod,
                                        nb_prod,
                                        (int)DECAF_CHANNEL_OK);

        MPI_Group group, newgroup;
        int range[3];
        range[0] = start_prod;
        range[1] = start_prod + nb_prod - 1;
        range[2] = 1;
        MPI_Comm_group(world_comm, &group);
        MPI_Group_range_incl(group, 1, &range, &newgroup);
        MPI_Comm_create_group(world_comm, newgroup, 0, &prod_comm_handle_);
        MPI_Group_free(&group);
        MPI_Group_free(&newgroup);
    }

    if(is_dflow())
    {
        channel_dflow_ = new OneWayChannel(  world_comm_,
                                        start_dflow,
                                        start_dflow,
                                        nb_dflow,
                                        (int)DECAF_CHANNEL_WAIT);

        MPI_Group group, newgroup;
        int range[3];
        range[0] = start_dflow;
        range[1] = start_dflow + nb_dflow - 1;
        range[2] = 1;
        MPI_Comm_group(world_comm, &group);
        MPI_Group_range_incl(group, 1, &range, &newgroup);
        MPI_Comm_create_group(world_comm, newgroup, 0, &dflow_comm_handle_);
        MPI_Group_free(&group);
        MPI_Group_free(&newgroup);

        buffer_ = new StorageMainMemory(buffer_max_size_);
    }

    if(is_con())
    {
        channel_con_ = new OneWayChannel(  world_comm_,
                                        start_con,
                                        start_con,
                                        nb_con,
                                        (int)DECAF_CHANNEL_OK);
        MPI_Group group, newgroup;
        int range[3];
        range[0] = start_con;
        range[1] = start_con + nb_con - 1;
        range[2] = 1;
        MPI_Comm_group(world_comm, &group);
        MPI_Group_range_incl(group, 1, &range, &newgroup);
        MPI_Comm_create_group(world_comm, newgroup, 0, &con_comm_handle_);
        MPI_Group_free(&group);
        MPI_Group_free(&newgroup);
    }

    if(is_con_root() || is_dflow_root())
    {
        channel_dflow_con_ = new OneWayChannel(world_comm_,
                                           start_con,
                                           start_dflow,
                                           1,
                                           (int)DECAF_CHANNEL_WAIT);
    }

    if(is_prod_root() || is_dflow_root())
    {
        channel_prod_dflow_ = new OneWayChannel(world_comm_,
                                                start_dflow,
                                                start_prod,
                                                1,
                                                (int)DECAF_CHANNEL_OK);
    }
}


void
decaf::
DatastreamDoubleFeedback::processProd(pConstructData data)
{
    bool blocking = false;
    if(is_prod_root())
    {
        DecafChannelCommand command_received;
        if(first_iteration_)
        {
            command_received = DECAF_CHANNEL_OK;
            first_iteration_ = false;
        }
        else
            command_received = channel_prod_dflow_->checkSelfCommand();
        if(command_received == DECAF_CHANNEL_WAIT)
        {
            //fprintf(stderr,"Blocking command received. We should block\n");
            //fprintf(stderr,"Broadcasting on prod root the value 1\n");

            channel_prod_->sendCommand(DECAF_CHANNEL_WAIT);
            blocking = true;
        }
    }
    MPI_Barrier(prod_comm_handle_);

    // TODO: remove busy wait
    DecafChannelCommand signal_prod;
    do
    {
        signal_prod = channel_prod_->checkSelfCommand();

        if(is_prod_root() && blocking == true)
        {
            DecafChannelCommand command_received = channel_prod_dflow_->checkSelfCommand();

            if(command_received == DECAF_CHANNEL_OK)
            {
                //fprintf(stderr,"Unblocking command received.\n");
                //fprintf(stderr,"Broadcasting on prod root the value 0\n");

                channel_prod_->sendCommand(DECAF_CHANNEL_OK);
                break;
            }
        }

        usleep(1000); //1ms


     } while(signal_prod != DECAF_CHANNEL_OK);

    // send the message
    redist_prod_dflow_->process(data, DECAF_REDIST_SOURCE);
    redist_prod_dflow_->flush();
}

void decaf::DatastreamDoubleFeedback::processDflow(pConstructData data)
{
    bool receivedDflowSignal = false;

    // First phase: Waiting for the signal and checking incoming msgs
    while(!receivedDflowSignal)
    {
        // Checking the root communication
        if(is_dflow_root())
        {
            bool receivedRootSignal;
            if(first_iteration_)
            {
                first_iteration_ = false;
                receivedRootSignal =  true;
            }
            else
                receivedRootSignal =  channel_dflow_con_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT);

            if(receivedRootSignal)
            {
                channel_dflow_->sendCommand(DECAF_CHANNEL_OK);
                framemanager_->computeNextFrame();
            }

        }

        // NOTE: Barrier could be removed if we used only async point-to-point communication
        // For now collective are creating deadlocks without barrier
        // Ex: 1 dflow do put while the other do a get
        MPI_Barrier(dflow_comm_handle_);

        receivedDflowSignal = channel_dflow_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT);
        //receivedDflowSignal = framemanager_->hasNextFrameId();

        if(doGet_ && !is_blocking_)
        {
            pConstructData container;
            bool received = redist_prod_dflow_->IGet(container);
            if(received)
            {
                if(msgtools::test_quit(container))
                    doGet_ = false;
                redist_prod_dflow_->flush();
                framemanager_->putFrame(iteration_);
                //buffer_.insert(std::pair<unsigned int,pConstructData>(iteration_, container));
                buffer_->insert(iteration_, container);
                iteration_++;

                // Block the producer if reaching the maximum buffer size
                //if(buffer_.size() >= buffer_max_size_ && !is_blocking_)
                if(!is_blocking_ && buffer_->isFull())
                {
                    is_blocking_ = true;

                    if(is_dflow_root())
                        channel_prod_dflow_->sendCommand(DECAF_CHANNEL_WAIT);
                }
            }
        }
    }

    while(!framemanager_->hasNextFrame())
    {
        if(!doGet_)
        {
            fprintf(stderr, "ERROR: trying to get after receiving a terminate msg.\n");
        }
        pConstructData container;
        redist_prod_dflow_->process(container, DECAF_REDIST_DEST);
        redist_prod_dflow_->flush();
        framemanager_->putFrame(iteration_);
        //buffer_.insert(std::pair<unsigned int,pConstructData>(iteration_, container));
        buffer_->insert(iteration_, container);
        iteration_++;

        // Block the producer if reaching the maximum buffer size
        //if(buffer_.size() >= buffer_max_size_ && !is_blocking_)
        if(!is_blocking_ && buffer_->isFull())
        {
            fprintf(stderr, "WARNING: Blocking the producer while waiting for incoming message. Can create a deadlock.\n");
            is_blocking_ = true;

            if(is_dflow_root())
                channel_prod_dflow_->sendCommand(DECAF_CHANNEL_WAIT);
        }
    }

    unsigned int frame_id;
    FrameCommand command = framemanager_->getNextFrame(&frame_id);
    //data->merge(buffer_.find(frame_id)->second.getPtr());
    //buffer_.erase(frame_id);
    data->merge(buffer_->getData(frame_id).getPtr());
    buffer_->erase(frame_id);



    // Unblock the producer if necessary
    //if(is_blocking_ && buffer_.size() < buffer_max_size_)
    if(is_blocking_ && !buffer_->isFull())
    {
        is_blocking_ = false;

        if(is_dflow_root())
            channel_prod_dflow_->sendCommand(DECAF_CHANNEL_OK);
    }
}

void
decaf::
DatastreamDoubleFeedback::processCon(pConstructData data)
{
    if(!is_con_root())
        return;

    channel_dflow_con_->sendCommand(DECAF_CHANNEL_OK);
}

} // namespace
#endif


