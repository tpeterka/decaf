#include <manala/datastream/datastreamsinglenolink.hpp>

decaf::
DatastreamSingleFeedbackNoLink::~DatastreamSingleFeedbackNoLink()
{
    if(channel_prod_) delete channel_prod_;
    if(channel_prod_con_) delete channel_prod_con_;
    if(channel_con_) delete channel_con_;
}

decaf::
DatastreamSingleFeedbackNoLink::DatastreamSingleFeedbackNoLink(CommHandle world_comm,
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
    channel_prod_(NULL), channel_prod_con_(NULL),channel_con_(NULL),
    first_iteration_(true), doGet_(true), is_blocking_(false), iteration_(0)
{
    fprintf(stderr,"ERROR: Stream with single feedback no link in not available with a link. Abording.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
}

decaf::
DatastreamSingleFeedbackNoLink::DatastreamSingleFeedbackNoLink(CommHandle world_comm,
           int start_prod,
           int nb_prod,
           int start_con,
           int nb_con,
           RedistComp* redist_prod_con,
           ManalaInfo& manala_info):
    Datastream(world_comm, start_prod, nb_prod, start_con, nb_con,
               redist_prod_con, manala_info),
    channel_prod_(NULL), channel_prod_con_(NULL),channel_con_(NULL),
    first_iteration_(true), doGet_(true), is_blocking_(false), iteration_(0)
{
    initialized_ = true;

    if(is_prod())
    {
        channel_prod_ = new OneWayChannel(  world_comm_,
                                        start_prod,
                                        start_prod,
                                        nb_prod,
                                        (int)DECAF_CHANNEL_WAIT);

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
        channel_prod_con_ = new OneWayChannel(world_comm_,
                                           start_con,
                                           start_prod,
                                           1,
                                           (int)DECAF_CHANNEL_WAIT);
    }
}


void
decaf::
DatastreamSingleFeedbackNoLink::processProd(pConstructData data)
{
    // First checking if we have to send the frame
    if(!msgtools::test_quit(data) && !framemanager_->sendFrame(iteration_))
    {
        iteration_++;
        return;
    }
    else
        iteration_++;

    bool receivedProdSignal = false;

    // First phase: Waiting for the signal from con
    while(!receivedProdSignal)
    {
        // Checking the root communication
        if(is_prod_root())
        {
            bool receivedRootSignal;
            if(first_iteration_)
            {
                first_iteration_ = false;
                receivedRootSignal =  true;
            }
            else
                receivedRootSignal =  channel_prod_con_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT);

            // We received the signal from the consumer
            // Broadcasting the information
            if(receivedRootSignal)
            {
                channel_prod_->sendCommand(DECAF_CHANNEL_OK);
            }

        }

        // NOTE: Barrier could be removed if we used only async point-to-point communication
        // For now collective are creating deadlocks without barrier
        // Ex: 1 dflow do put while the other do a get
        //MPI_Barrier(dflow_comm_handle_);

        receivedProdSignal = channel_prod_->checkAndReplaceSelfCommand(DECAF_CHANNEL_OK, DECAF_CHANNEL_WAIT);

        if(!receivedProdSignal)
            usleep(100);
    }

    // send the message
    redist_prod_con_->process(data, DECAF_REDIST_SOURCE);
    redist_prod_con_->flush();
}

void
decaf::
DatastreamSingleFeedbackNoLink::processDflow(pConstructData data)
{
    fprintf(stderr, "ERROR: calling processDflow on a single feedback stream without link. Abording.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
}

void
decaf::
DatastreamSingleFeedbackNoLink::processCon(pConstructData data)
{
    if(!is_con_root())
        return;

    channel_prod_con_->sendCommand(DECAF_CHANNEL_OK);
}
