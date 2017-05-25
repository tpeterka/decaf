#include <manala/datastream/datastreamsinglefeedback.hpp>

decaf::
DatastreamSingleFeedback::~DatastreamSingleFeedback()
{
    if(channel_dflow_) delete channel_dflow_;
    if(channel_dflow_con_) delete channel_dflow_con_;
    if(channel_con_) delete channel_con_;
}

decaf::
DatastreamSingleFeedback::DatastreamSingleFeedback(CommHandle world_comm,
       int start_prod,
       int nb_prod,
       int start_dflow,
       int nb_dflow,
       int start_con,
       int nb_con,
       RedistComp* prod_dflow,
       RedistComp* dflow_con,
       ManalaInfo& manala_info):
    Datastream(world_comm, start_prod, nb_prod, start_dflow, nb_dflow, start_con, nb_con, prod_dflow, dflow_con, manala_info),
    channel_dflow_(NULL), channel_dflow_con_(NULL),channel_con_(NULL),
    first_iteration_(true), doGet_(true), is_blocking_(false), iteration_(0)
{
    initialized_ = true;

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
}

decaf::
DatastreamSingleFeedback::DatastreamSingleFeedback(CommHandle world_comm,
           int start_prod,
           int nb_prod,
           int start_con,
           int nb_con,
           RedistComp* redist_prod_con,
           ManalaInfo& manala_info)
{
    fprintf(stderr,"ERROR: Stream with single feedback in not available without a link. Abording.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
}


void
decaf::
DatastreamSingleFeedback::processProd(pConstructData data)
{
    // First checking if we have to send the frame
    if(!msgtools::test_quit(data) && !framemanager_->sendFrame(iteration_))
    {
        iteration_++;
        return;
    }
    else
        iteration_++;

    // send the message
    redist_prod_dflow_->process(data, DECAF_REDIST_SOURCE);
    redist_prod_dflow_->flush();
}

void decaf::DatastreamSingleFeedback::processDflow(pConstructData data)
{
    bool receivedDflowSignal = false;

    // First phase: Waiting for the signal from con and checking incoming msgs
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

        if(doGet_)
        {
            pConstructData container;
            bool received = redist_prod_dflow_->IGet(container);
            if(received)
            {
                if(msgtools::test_quit(container))
                    doGet_ = false;
                redist_prod_dflow_->flush();
                framemanager_->putFrame(iteration_);
                storage_collection_->insert(iteration_, container);

                iteration_++;
            }
        }
    }

    // Check if we receive the iteration to send and if we received the corresonding frame
    while(!framemanager_->hasNextFrame())
    {
        if(doGet_)
        {
            pConstructData container;
            redist_prod_dflow_->process(container, DECAF_REDIST_DEST);
            redist_prod_dflow_->flush();
            framemanager_->putFrame(iteration_);
            storage_collection_->insert(iteration_, container);
            iteration_++;

            if(msgtools::test_quit(container))
            {
                doGet_ = false;
                //fprintf(stderr, "Reception of the terminate message. Saving data on file.\n");
                //storage_collection_->save(world_rank_);
            }
        }
        usleep(100);
    }

    unsigned int frame_id;
    FrameCommand command = framemanager_->getNextFrame(&frame_id);
    data->merge(storage_collection_->getData(frame_id).getPtr());
    storage_collection_->processCommand(command, frame_id);
}

void
decaf::
DatastreamSingleFeedback::processCon(pConstructData data)
{
    if(first_iteration_)
    {
       first_iteration_ = false;
       return;
    }
    if(!is_con_root())
        return;

    channel_dflow_con_->sendCommand(DECAF_CHANNEL_OK);
}
