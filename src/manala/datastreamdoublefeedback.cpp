#include <manala/datastream/datastreamdoublefeedback.hpp>

decaf::
DatastreamDoubleFeedback::~DatastreamDoubleFeedback()
{
    if(channel_prod_) delete channel_prod_;
    if(channel_prod_dflow_) delete channel_prod_dflow_;
    if(channel_dflow_prod_) delete channel_dflow_prod_;
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
                                                   RedistComp* dflow_con,
                                                   ManalaInfo& manala_info):
    Datastream(world_comm, start_prod, nb_prod, start_dflow, nb_dflow, start_con, nb_con, prod_dflow, dflow_con, manala_info),
    channel_prod_(NULL), channel_prod_dflow_(NULL), channel_dflow_(NULL), channel_dflow_con_(NULL),channel_con_(NULL),
    first_iteration_(true), doGet_(true), is_blocking_(false), iteration_(0)
{
    initialized_ = true;

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
        channel_dflow_prod_ = new OneWayChannel(world_comm_,
                                                start_dflow,
                                                start_prod,
                                                1,
                                                (int)DECAF_CHANNEL_WAIT);
        channel_prod_dflow_ = new OneWayChannel(world_comm_,
                                                start_prod,
                                                start_dflow,
                                                1,
                                                (int)DECAF_CHANNEL_OK);
    }
}
decaf::
DatastreamDoubleFeedback::DatastreamDoubleFeedback(CommHandle world_comm,
                                                   int start_prod,
                                                   int nb_prod,
                                                   int start_con,
                                                   int nb_con,
                                                   RedistComp* redist_prod_con,
                                                   ManalaInfo& manala_info)
{
    fprintf(stderr,"ERROR: Stream with double feedback in not available without a link. Abording.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
}


void
decaf::
DatastreamDoubleFeedback::processProd(pConstructData data)
{
    // First checking if we have to send the frame
    if(!msgtools::test_quit(data) && !framemanager_->sendFrame(iteration_))
    {
        iteration_++;
        return;
    }
    else
        iteration_++;

    if(is_prod_root())
    {
        //Sending the request to the link
        channel_prod_dflow_->sendCommand(DECAF_CHANNEL_WAIT);
        //fprintf(stderr, "Sending the command to the dflow. Waiting for the Ack from the link...\n");

        // Waiting for the ack from the link
        while(!channel_dflow_prod_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT))
            usleep(100);
        //fprintf(stderr, "Ack from the link received. Unblocking the other prods...\n");

        // Broadcasting the ack to all the producers
        channel_prod_->sendCommand(DECAF_CHANNEL_OK);
    }

    // TODO: check without barrier
    MPI_Barrier(prod_comm_handle_);
    //fprintf(stderr,"Barrier producer passed.\n");

    // Waiting for the ack from the root producer
    while(!channel_prod_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT))
        usleep(100);
    //fprintf(stderr, "Received the order from the root produer. Puting...\n");

    // send the message
    redist_prod_dflow_->process(data, DECAF_REDIST_SOURCE);
    redist_prod_dflow_->flush();
    //fprintf(stderr, "Message put\n");
}

void decaf::DatastreamDoubleFeedback::processDflow(pConstructData data)
{
    bool receivedDflowSignal = false;
    channel_dflow_->updateSelfValue((int)DECAF_CHANNEL_WAIT);

    // First phase: Waiting for the signal and checking incoming msgs
    while(!receivedDflowSignal)
    {
        // Root node: checking the signal from both the producer and consumer
        if(is_dflow_root())
        {
            bool waitingProd = false;
            bool sendCon = false;
            while(!waitingProd && !sendCon)
            {
                // TODO: keep track of the last command processed.
                // Risk to only get but never put
                if(doGet_ && !storage_collection_->isFull())
                    waitingProd = channel_prod_dflow_->checkAndReplaceSelfCommand(DECAF_CHANNEL_WAIT, DECAF_CHANNEL_OK);
                if(waitingProd)
                {
                    channel_dflow_prod_->sendCommand(DECAF_CHANNEL_OK);
                    channel_dflow_->sendCommand(DECAF_CHANNEL_GET);
                    //fprintf(stderr, "Reception of a producer notification. Sending the get command.\n");
                }
                else if(storage_collection_->getNbDataStored() != 0)
                {
                    sendCon = channel_dflow_con_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT);
                    if(sendCon)
                    {
                        channel_dflow_->sendCommand(DECAF_CHANNEL_PUT);
                        framemanager_->computeNextFrame();
                        //fprintf(stderr, "Reception of a consumer notification. Sending the put command\n");
                    }
                }
            }
        }
        MPI_Barrier(dflow_comm_handle_);

        // Receiving a command on all the dflows
        DecafChannelCommand cmd =  channel_dflow_->checkSelfCommand();

        switch (cmd) {
        case DECAF_CHANNEL_GET:
        {
            //fprintf(stderr, "Processing a get command.\n");
            if(doGet_)
            {
                pConstructData container;
                bool received = redist_prod_dflow_->IGet(container);
                if(received)
                {
                    if(msgtools::test_quit(container))
                    {
                        doGet_ = false;
                        //fprintf(stderr, "Reception of the terminate message. Saving data on file.\n");
                        //storage_collection_->save(world_rank_);
                    }
                    redist_prod_dflow_->flush();
                    framemanager_->putFrame(iteration_);
                    storage_collection_->insert(iteration_, container);
                    iteration_++;
                }
                //fprintf(stderr, "Reception of a message from the producer completed.\n");
            }
            break;
        }
        case DECAF_CHANNEL_PUT:
        {
            //fprintf(stderr, "Processing a put command.\n");
            if(!framemanager_->hasNextFrame())
            {
                fprintf(stderr, "ERROR: the frame requested has not arrived yet.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
            }

            unsigned int frame_id;
            FrameCommand command = framemanager_->getNextFrame(&frame_id);
            data->merge(storage_collection_->getData(frame_id).getPtr());
            storage_collection_->processCommand(command, frame_id);
            receivedDflowSignal = true;
        }
        default:
            break;
        }
        channel_dflow_->updateSelfValue((int)DECAF_CHANNEL_WAIT);
    }
}

void
decaf::
DatastreamDoubleFeedback::processCon(pConstructData data)
{
    if(!is_con_root())
        return;
    //fprintf(stderr, "Consumer is ready. Sending the request.\n");
    channel_dflow_con_->sendCommand(DECAF_CHANNEL_OK);
}
