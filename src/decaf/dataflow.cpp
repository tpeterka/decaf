#include <decaf/dataflow.hpp>

DecafSizes*
decaf::
Dataflow::sizes()
{
    return &sizes_;
}

void
decaf::
Dataflow::err()
{
    ::all_err(err_);
}

// whether this rank is producer, dataflow, or consumer (may have multiple roles)
bool
decaf::
Dataflow::is_prod()
{
    return((type_ & DECAF_PRODUCER_COMM) == DECAF_PRODUCER_COMM);
}

bool
decaf::
Dataflow::is_dflow()
{
    return((type_ & DECAF_DATAFLOW_COMM) == DECAF_DATAFLOW_COMM);
}

bool
decaf::
Dataflow::is_con()
{
    return((type_ & DECAF_CONSUMER_COMM) == DECAF_CONSUMER_COMM);
}

bool
decaf::
Dataflow::is_prod_root()
{
    return world_rank_ == sizes_.prod_start;
}

bool
decaf::
Dataflow::is_dflow_root()
{
    return world_rank_ == sizes_.dflow_start;
}

bool
decaf::
Dataflow::is_con_root()
{
    return world_rank_ == sizes_.con_start;
}

CommHandle
decaf::
Dataflow::prod_comm_handle()
{
    return prod_comm_->handle();
}

CommHandle
decaf::
Dataflow::con_comm_handle()
{
    return con_comm_->handle();
}

CommHandle
decaf::
Dataflow::dflow_comm_handle()
{
    return dflow_comm_->handle();
}

Comm*
decaf::
Dataflow::prod_comm()
{
    return prod_comm_;
}

Comm*
decaf::
Dataflow::dflow_comm()
{
    return dflow_comm_;
}

Comm*
decaf::
Dataflow::con_comm()
{
    return con_comm_;
}

vector<ContractKey>&
decaf::
Dataflow::keys()
{
    return keys_;
}

bool
decaf::
Dataflow::has_contract()
{
    return bContract_;
}

vector<ContractKey>&
decaf::
Dataflow::keys_link()
{
    return keys_link_;
}

bool
decaf::
Dataflow::has_contract_link()
{
    return bContractLink_;
}

bool
decaf::
Dataflow::isAny()
{
    return bAny_;
}

string
decaf::
Dataflow::srcPort()
{
    return srcPort_;
}

string
decaf::
Dataflow::destPort()
{
    return destPort_;
}

bool
decaf::
Dataflow::isClose()
{
    return bClose_;
}

void
decaf::
Dataflow::setClose()
{
    bClose_ = true;
}

RedistComp* createRedistComponent(Decomposition decomp, TransportMethod transport,
                                  int prod_start, int prod_size,
                                  int con_start, int con_size,
                                  int world_rank, CommHandle world_comm,
                                  string& redist_name, RedistCommMethod redist_method)
{
    RedistComp* result;
    switch(decomp)
    {
    case DECAF_ROUND_ROBIN_DECOMP:
    {
        switch (transport)
        {
        case DECAF_TRANSPORT_MPI:
        {
#ifdef TRANSPORT_MPI
            // fprintf(stderr, "Using Round for prod -> dflow\n");
            result = new RedistRoundMPI(    prod_start,
                                            prod_size,
                                            con_start,
                                            con_size,
                                            world_comm,
                                            redist_method);
#else
            fprintf(stderr, "ERROR: Requesting MPI transport method but Bredala was not compiled with MPI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_CCI:
        {
#ifdef TRANSPORT_CCI
            result = new RedistRoundCCI(prod_start,
                                        prod_size,
                                        con_start,
                                        con_size,
                                        world_rank,
                                        world_comm,
                                        redist_name,
                                        DECAF_REDIST_P2P);
#else
            fprintf(stderr, "ERROR: Requesting CCI transport method but Bredala was not compiled with CCI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_FILE:
        {
#ifdef TRANSPORT_FILE
            result = new RedistRoundFile(prod_start,
                                         prod_size,
                                         con_start,
                                         con_size,
                                         world_rank,
                                         world_comm,
                                         redist_name,
                                         DECAF_REDIST_COLLECTIVE);
#else
            fprintf(stderr, "ERROR: Requesting File transport method but Bredala was not compiled with File transport method.");
            exit(-1);
#endif
            break;
        }
        default:
            fprintf(stderr,"ERROR: Transport method not implemented.\n");
            exit(-1);
        break;
        }
        break;
    }
    case DECAF_CONTIG_DECOMP:
    {
        switch (transport)
        {
        case DECAF_TRANSPORT_MPI:
        {
#ifdef TRANSPORT_MPI
            // fprintf(stderr, "Using Round for prod -> dflow\n");
            result = new RedistCountMPI(    prod_start,
                                            prod_size,
                                            con_start,
                                            con_size,
                                            world_comm,
                                            redist_method);
#else
            fprintf(stderr, "ERROR: Requesting MPI transport method but Bredala was not compiled with MPI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_CCI:
        {
#ifdef TRANSPORT_CCI
            result = new RedistCountCCI(prod_start,
                                        prod_size,
                                        con_start,
                                        con_size,
                                        world_rank,
                                        world_comm,
                                        redist_name,
                                        DECAF_REDIST_P2P);
#else
            fprintf(stderr, "ERROR: Requesting CCI transport method but Bredala was not compiled with CCI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_FILE:
        {
#ifdef TRANSPORT_FILE
            result = new RedistCountFile(prod_start,
                                         prod_size,
                                         con_start,
                                         con_size,
                                         world_rank,
                                         world_comm,
                                         redist_name,
                                         DECAF_REDIST_COLLECTIVE);
#else
            fprintf(stderr, "ERROR: Requesting File transport method but Bredala was not compiled with File transport method.");
            exit(-1);
#endif
            break;
        }
        default:
            fprintf(stderr,"ERROR: Transport method not implemented.\n");
            exit(-1);
        break;
        }
        break;
    }

    case DECAF_BLOCK_DECOMP:
    {
        switch (transport)
        {
        case DECAF_TRANSPORT_MPI:
        {
#ifdef TRANSPORT_MPI
            // fprintf(stderr, "Using Round for prod -> dflow\n");
            result = new RedistBlockMPI(    prod_start,
                                            prod_size,
                                            con_start,
                                            con_size,
                                            world_comm,
                                            redist_method);
#else
            fprintf(stderr, "ERROR: Requesting MPI transport method but Bredala was not compiled with MPI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_CCI:
        {
#ifdef TRANSPORT_CCI
            result = new RedistBlockCCI(prod_start,
                                        prod_size,
                                        con_start,
                                        con_size,
                                        world_rank,
                                        world_comm,
                                        redist_name,
                                        DECAF_REDIST_P2P);
#else
            fprintf(stderr, "ERROR: Requesting CCI transport method but Bredala was not compiled with CCI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_FILE:
        {
#ifdef TRANSPORT_FILE
            result = new RedistBlockFile(prod_start,
                                         prod_size,
                                         con_start,
                                         con_size,
                                         world_rank,
                                         world_comm,
                                         redist_name,
                                         DECAF_REDIST_COLLECTIVE);
#else
            fprintf(stderr, "ERROR: Requesting File transport method but Bredala was not compiled with File transport method.");
            exit(-1);
#endif
            break;
        }
        default:
            fprintf(stderr,"ERROR: Transport method not implemented.\n");
            exit(-1);
        break;
        }
        break;
    }
    case DECAF_PROC_DECOMP:
    {
        switch (transport)
        {
        case DECAF_TRANSPORT_MPI:
        {
#ifdef TRANSPORT_MPI
            // fprintf(stderr, "Using Round for prod -> dflow\n");
            result = new RedistProcMPI(    prod_start,
                                           prod_size,
                                           con_start,
                                           con_size,
                                           world_comm,
                                           redist_method);
#else
            fprintf(stderr, "ERROR: Requesting MPI transport method but Bredala was not compiled with MPI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_CCI:
        {
#ifdef TRANSPORT_CCI
            result = new RedistProcCCI(prod_start,
                                       prod_size,
                                       con_start,
                                       con_size,
                                       world_rank,
                                       world_comm,
                                       redist_name,
                                       DECAF_REDIST_P2P);
#else
            fprintf(stderr, "ERROR: Requesting CCI transport method but Bredala was not compiled with CCI transport method.");
            exit(-1);
#endif
            break;
        }
        case DECAF_TRANSPORT_FILE:
        {
#ifdef TRANSPORT_FILE
            result = new RedistProcFile(prod_start,
                                        prod_size,
                                        con_start,
                                        con_size,
                                        world_rank,
                                        world_comm,
                                        redist_name,
                                        DECAF_REDIST_COLLECTIVE);
#else
            fprintf(stderr, "ERROR: Requesting File transport method but Bredala was not compiled with File transport method.");
            exit(-1);
#endif
            break;
        }
        default:
            fprintf(stderr,"ERROR: Transport method not implemented.\n");
            exit(-1);
        break;
        }
        break;
    }
    default:
    {
        fprintf(stderr, "ERROR: policy %d unrecognized to select a redistribution component. "
                        "Abording.\n", decomp);
        exit(-1);
        break;
    }

    }

    return result;
}


decaf::
Dataflow::Dataflow(CommHandle world_comm,
                   int workflow_size,
                   int workflow_rank,
                   DecafSizes& decaf_sizes,
                   int prod,
                   int dflow,
                   int con,
                   WorkflowLink wflowLink,
                   Decomposition prod_dflow_redist,
                   Decomposition dflow_cons_redist):
    world_comm_(world_comm),
    world_size_(workflow_size),
    world_rank_(workflow_rank),
    sizes_(decaf_sizes),
    wflow_prod_id_(prod),
    wflow_dflow_id_(dflow),
    wflow_con_id_(con),
    redist_prod_dflow_(NULL),
    redist_dflow_con_(NULL),
    redist_prod_con_(NULL),
    type_(DECAF_OTHER_COMM),
    check_level_(wflowLink.check_level),
    srcPort_(wflowLink.srcPort),
    destPort_(wflowLink.destPort),
    tokens_(0),
    bAny_(wflowLink.bAny),
    iteration(0),
    bClose_(false),
    no_link_(false),
    use_stream_(false)
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

    TransportMethod transport = stringToTransportMethod(wflowLink.transport_method);

    // Fetching the global size of the workflow
    /*const char* env_decaf_size = std::getenv("DECAF_WORKFLOW_SIZE");
    if (env_decaf_size != NULL)
        world_size_ = atoi(env_decaf_size);
    else
        world_size_ = CommSize(world_comm);

    // Fethcing the global rank of the current process
    // The first rank is the global rank in the workflow
    // The world_rank is then the rank of the process in the local communicator
    // plus the rank of the first rank
    const char* env_first_rank = std::getenv("DECAF_WORKFLOW_RANK");
    int global_first_rank = 0;
    if (env_first_rank != NULL)
        global_first_rank = atoi(env_first_rank);
    world_rank_ = CommRank(world_comm) + global_first_rank; // my place in the world
    */

    // ensure sizes and starts fit in the world
    if (sizes_.prod_start + sizes_.prod_size > world_size_   ||
            sizes_.dflow_start + sizes_.dflow_size > world_size_ ||
            sizes_.con_start + sizes_.con_size > world_size_)
    {
        err_ = DECAF_COMM_SIZES_ERR;
        return;
    }

    // Sets the boolean value to true/false whether the dataflow is related to a contract or not
    if(wflowLink.list_keys.size() == 0)
        bContract_ = false;
    else
    {
        bContract_ = true;
        keys_ = wflowLink.list_keys;

    }
    if (wflowLink.keys_link.size() == 0)
    {
        bContractLink_ = false;
    }
    else
    {
        bContractLink_ = true;
        keys_link_ = wflowLink.keys_link;
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
        if (transport == DECAF_TRANSPORT_MPI)
            prod_comm_ = new Comm(world_comm, sizes_.prod_start, sizes_.prod_start + sizes_.prod_size - 1);
        else if (transport == DECAF_TRANSPORT_CCI)
            prod_comm_ = new Comm(world_comm);
        else if (transport == DECAF_TRANSPORT_FILE)
            prod_comm_ = new Comm(world_comm);
        else
        {
            fprintf(stderr, "ERROR: Transport method not implemented.\n");
            err_ = DECAF_TRANSPORT_METHOD;
            return;
        }
    }

    if (world_rank_ >= sizes_.dflow_start &&                  // dataflow
            world_rank_ < sizes_.dflow_start + sizes_.dflow_size)
    {
        type_ |= DECAF_DATAFLOW_COMM;
        if (transport == DECAF_TRANSPORT_MPI)
            dflow_comm_ = new Comm(world_comm, sizes_.dflow_start, sizes_.dflow_start + sizes_.dflow_size - 1);
        else if (transport == DECAF_TRANSPORT_CCI)
            dflow_comm_ = new Comm(world_comm);
        else if (transport == DECAF_TRANSPORT_FILE)
            dflow_comm_ = new Comm(world_comm);
        else
        {
            fprintf(stderr, "ERROR: Transport method not implemented.\n");
            err_ = DECAF_TRANSPORT_METHOD;
            return;
        }
    }

    if (world_rank_ >= sizes_.con_start &&                    // consumer
            world_rank_ < sizes_.con_start + sizes_.con_size)
    {
        type_ |= DECAF_CONSUMER_COMM;
        if (transport == DECAF_TRANSPORT_MPI)
            con_comm_ = new Comm(world_comm, sizes_.con_start, sizes_.con_start + sizes_.con_size - 1);
        else if (transport == DECAF_TRANSPORT_CCI)
            con_comm_ = new Comm(world_comm);
        else if (transport == DECAF_TRANSPORT_FILE)
            con_comm_ = new Comm(world_comm);
        else
        {
            fprintf(stderr, "ERROR: Transport method not implemented.\n");
            err_ = DECAF_TRANSPORT_METHOD;
            return;
        }

        tokens_ = wflowLink.tokens;
    }

    if( (type_ & DECAF_PRODUCER_COMM) == DECAF_PRODUCER_COMM && (type_ & DECAF_DATAFLOW_COMM) == DECAF_DATAFLOW_COMM)
    {
        prod_link_overlap = true;
    }
    else
    {
        prod_link_overlap = false;
    }

    // producer and dataflow
    if (sizes_.dflow_size > 0 && (
                (world_rank_ >= sizes_.prod_start && world_rank_ < sizes_.prod_start + sizes_.prod_size) ||
                (world_rank_ >= sizes_.dflow_start && world_rank_ < sizes_.dflow_start + sizes_.dflow_size)))
    {
        string redist_name = wflowLink.name + string("_pd");
        redist_prod_dflow_ = createRedistComponent(prod_dflow_redist,
                                                   transport,
                                                   sizes_.prod_start,
                                                   sizes_.prod_size,
                                                   sizes_.dflow_start,
                                                   sizes_.dflow_size,
                                                   world_rank_,
                                                   world_comm,
                                                   redist_name,
                                                   DECAF_REDIST_COLLECTIVE);
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
        string redist_name = wflowLink.name + string("_dc");
        redist_dflow_con_ = createRedistComponent(  dflow_cons_redist,
                                                    transport,
                                                    sizes_.dflow_start,
                                                    sizes_.dflow_size,
                                                    sizes_.con_start,
                                                    sizes_.con_size,
                                                    world_rank_,
                                                    world_comm,
                                                    redist_name,
                                                    DECAF_REDIST_COLLECTIVE);
    }
    else
        //fprintf(stderr, "No redistribution between dflow and cons needed.\n");
        redist_dflow_con_ = NULL;

    // producer and consumer
    if (sizes_.dflow_size == 0 && (
                (world_rank_ >= sizes_.prod_start && world_rank_ < sizes_.prod_start + sizes_.prod_size) ||
                (world_rank_ >= sizes_.con_start && world_rank_ < sizes_.con_start + sizes_.con_size)))
    {
        no_link_ = true;
        bContractLink_ = false; //TO be sure

        string redist_name = wflowLink.name + string("_pc");
        redist_prod_con_ = createRedistComponent(   prod_dflow_redist,
                                                    transport,
                                                    sizes_.prod_start,
                                                    sizes_.prod_size,
                                                    sizes_.con_start,
                                                    sizes_.con_size,
                                                    world_rank_,
                                                    world_comm,
                                                    redist_name,
                                                    DECAF_REDIST_COLLECTIVE);
    }
    else
        redist_prod_con_ = NULL;

    stream_policy_ = stringToStreamPolicy(wflowLink.manala_info.stream);

    if (transport != DECAF_TRANSPORT_MPI && stream_policy_ != DECAF_STREAM_NONE)
    {
        fprintf(stderr, "Manala only available with MPI transport layer. Turning off.\n");
        stream_policy_ = DECAF_STREAM_NONE;
    }

    if ( stream_policy_ != DECAF_STREAM_NONE &&
            ((!no_link_ && wflowLink.manala_info.storages.size() > 0) || (no_link_ && stream_policy_ == DECAF_STREAM_SINGLE)))
    {
        fprintf(stderr, "Manala activated.\n");
        use_stream_ = true;
    }

    // Buffering setup
    if(use_stream_)
    {
        switch(stream_policy_)
        {
        case DECAF_STREAM_SINGLE:
        {
            if(no_link_)
                stream_ = new DatastreamSingleFeedbackNoLink(
                            world_comm_,
                            sizes_.prod_start,
                            sizes_.prod_size,
                            sizes_.con_start,
                            sizes_.con_size,
                            redist_prod_con_,
                            wflowLink.manala_info);
            else
                stream_ = new DatastreamSingleFeedback(
                            world_comm_,
                            sizes_.prod_start,
                            sizes_.prod_size,
                            sizes_.dflow_start,
                            sizes_.dflow_size,
                            sizes_.con_start,
                            sizes_.con_size,
                            redist_prod_dflow_,
                            redist_dflow_con_,
                            wflowLink.manala_info);

            break;
        }
        case DECAF_STREAM_DOUBLE:
        {
            stream_ = new DatastreamDoubleFeedback(
                        world_comm_,
                        sizes_.prod_start,
                        sizes_.prod_size,
                        sizes_.dflow_start,
                        sizes_.dflow_size,
                        sizes_.con_start,
                        sizes_.con_size,
                        redist_prod_dflow_,
                        redist_dflow_con_,
                        wflowLink.manala_info);

            break;
        }
        default:
        {
            fprintf(stderr, "ERROR: Unknown Stream policy. Disabling streaming.\n");
            use_stream_ = false;
            break;
        }
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

    if (redist_prod_con_)
        delete redist_prod_con_;

    if(stream_) delete stream_;
}

// puts data into the dataflow
// tags the data with the:
// - source type (producer/consumer or dataflow)
// - workflow link id corresponding to this dataflow
// - destination workflow node id or link id, depending on type of destination
// NB: source is *link* id while destination is *either node or link* id
// Returns true if ok, false if an ERROR occured
bool
decaf::
Dataflow::put(pConstructData& data, TaskType role)
{
    pConstructData data_filtered; // container to be sent
    bool data_removed_by_period; // i.e.: Has some data field been removed due to the periodicity during the call of filterAndCheck?
    bool data_filtered_empty;

    data_filtered = filterPut(data, data_filtered, role, data_removed_by_period, data_filtered_empty);

    // If the filtered message does not contain user data, no need to send
    if (data_filtered_empty){
        iteration++;
        return false;
    }

    if (role==DECAF_LINK || !prod_link_overlap)
        iteration++;


    data_filtered->removeData("src_type");
    data_filtered->removeData("link_id");
    data_filtered->removeData("dest_id");

    // encode type into message (producer/consumer or dataflow)
    shared_ptr<SimpleConstructData<TaskType> > value  =
            make_shared<SimpleConstructData<TaskType> >(role);
    data_filtered->appendData(string("src_type"), value,
                              DECAF_NOFLAG, DECAF_SYSTEM,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


    // encode dataflow link id into message
    shared_ptr<SimpleConstructData<int> > value1  =
            make_shared<SimpleConstructData<int> >(wflow_dflow_id_);
    data_filtered->appendData(string("link_id"), value1,
                              DECAF_NOFLAG, DECAF_SYSTEM,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


    if (role == DECAF_NODE)
    {
        if (no_link_)
        {
            // encode destination link id into message
            shared_ptr<SimpleConstructData<int> > value2  =
                    make_shared<SimpleConstructData<int> >(wflow_con_id_);
            data_filtered->appendData(string("dest_id"), value2,
                                      DECAF_NOFLAG, DECAF_SYSTEM,
                                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

            // send the message
            if (redist_prod_con_ == NULL)
            {
                fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
            if (data_removed_by_period)
                redist_prod_con_->clearBuffers(); // We clear the buffers before and after the sending

            // send the message
            if (use_stream_)
                stream_->processProd(data_filtered);
            else
            {
                redist_prod_con_->process(data_filtered, DECAF_REDIST_SOURCE);
                redist_prod_con_->flush();
            }
            if (data_removed_by_period)
                redist_prod_con_->clearBuffers();
        }
        else
        {
            // encode destination link id into message
            shared_ptr<SimpleConstructData<int> > value2  =
                    make_shared<SimpleConstructData<int> >(wflow_dflow_id_);
            data_filtered->appendData(string("dest_id"), value2,
                                      DECAF_NOFLAG, DECAF_SYSTEM,
                                      DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);

            if (redist_prod_dflow_ == NULL)
            {
                fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
            if (data_removed_by_period)
                redist_prod_dflow_->clearBuffers(); // We clear the buffers before and after the sending

            // send the message
            if (use_stream_)
                stream_->processProd(data_filtered);
            else
            {
                redist_prod_dflow_->process(data_filtered, DECAF_REDIST_SOURCE);
                redist_prod_dflow_->flush();

            }

            if (data_removed_by_period)
                redist_prod_dflow_->clearBuffers();

        }
    }
    else if (role == DECAF_LINK)
    {
        // encode destination node id into message
        shared_ptr<SimpleConstructData<int> > value2  =
                make_shared<SimpleConstructData<int> >(wflow_con_id_);
        data_filtered->appendData(string("dest_id"), value2,
                                  DECAF_NOFLAG, DECAF_SYSTEM,
                                  DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);


        // send the message
        if (redist_dflow_con_ == NULL)
        {
            fprintf(stderr, "Dataflow::put() trying to access a null communicator\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        if (data_removed_by_period)
            redist_dflow_con_->clearBuffers(); // We clear the buffers before and after the sending

        redist_dflow_con_->process(data_filtered, DECAF_REDIST_SOURCE);
        redist_dflow_con_->flush();
        if (data_removed_by_period)
            redist_dflow_con_->clearBuffers();

    }

    data_filtered->removeData("src_type");
    data_filtered->removeData("link_id");
    data_filtered->removeData("dest_id");
    data_filtered->removeData("decaf_iteration");

    return true;
}


// gets data from the dataflow
// return true if ok, false if an ERROR occured
bool
decaf::
Dataflow::get(pConstructData& data, TaskType role)
{
    if (role == DECAF_LINK)
    {
        if (redist_prod_dflow_ == NULL)
        {
            fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        if(use_stream_)
            stream_->processDflow(data);
        else    // No buffer, blocking get
        {
            redist_prod_dflow_->process(data, DECAF_REDIST_DEST);
            redist_prod_dflow_->flush();
        }

    }
    else if (role == DECAF_NODE)
    {
        // Checking if we have a token. If yes, we directly return without messages
        if(tokens_ > 0)
        {
            tokens_--;
            data->setToken(true);
            return true;
        }

        // Comnsumer side
        if(use_stream_)
            stream_->processCon(data);

        if(no_link_)
        {
            if (redist_prod_con_ == NULL)
            {
                fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
            redist_prod_con_->process(data, DECAF_REDIST_DEST);
            redist_prod_con_->flush();

            // Comnsumer side
            if(use_stream_)
                stream_->processCon(data);
        }
        else
        {
            if (redist_dflow_con_ == NULL)
            {
                fprintf(stderr, "Dataflow::get() trying to access a null communicator\n");
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
            redist_dflow_con_->process(data, DECAF_REDIST_DEST);
            redist_dflow_con_->flush();

            // Comnsumer side
            //if(use_stream_)
            //	stream_->processCon(data);
        }


        if(msgtools::test_quit(data))
        {
            setClose();
            return true;
        }
    }

    //TODO remove the 3 data src_type, link_id and dest_id? Useless for the user, he doesn't need to know that
    // Performs the filtering and checking of data if needed
    filterGet(data, role);

    return true;
}



decaf::pConstructData&
decaf::
Dataflow::filterPut(pConstructData& data, pConstructData& data_filtered, TaskType& role, bool& data_changed, bool& filtered_empty){
    data_changed = data->isEmpty();
    filtered_empty = false;

    if (data->hasData("decaf_quit")){
        msgtools::set_quit(data_filtered);
        return data_filtered;
    }

    if (data->isEmpty() || check_level_ < CHECK_PY_AND_SOURCE){ // If there is no user data or if there is no need to perform the check/filtering
        return data;
    }



    if (has_contract() || has_contract_link())
    {
        // iteration value is wrong if filtered data leads to an empty message at producer put, need to update it with the message from the producer node
        if(role == DECAF_LINK)
        {
            int i = msgtools::get_iteration(data);
            if (i != -1){
                iteration = i;
            }
        }
        string typeName;
        vector<ContractKey> &list_keys = (role == DECAF_LINK && has_contract_link()) ? keys_link() : keys() ;
        if (isAny())
        {   // No filtering of other fields, just need to check if the contract is respected
            // TODO This is an ugly way of doing this, by adding all fields then removing the ones not needed during the current iteration
            // maybe a better solution? In terms of cost, number of operations, ...
            for (string key : data->listUserKeys())
                // Append all user data from data to data_filtered.
                data_filtered->appendData(data, key);

            // Then check the contracts
            for (ContractKey field : list_keys)
            {
                if ( (iteration % field.period) == 0) // This field is sent during this iteration
                {
                    if (data->getTypename(field.name, typeName)) //Also checks if the field is present in the data model
                    {
                        // Performing typechecking
                        if (typeName.compare(field.type) != 0) //The two types do not match
                        {
                            string r = role == DECAF_NODE ? "Node " : "Link ";
                            r+= to_string(CommRank(world_comm_));
                            fprintf(stderr, "ERROR in %s: Contract not respected, sent type %s does not match the type %s of the field \"%s\". Aborting.\n", r.c_str(), typeName.c_str(), field.type.c_str(), field.name.c_str());
                            MPI_Abort(MPI_COMM_WORLD, 0);
                        }
                    }
                    else // If the field is not present in data the contract is not respected.
                    {
                        string r = role == DECAF_NODE ? "Node " : "Link ";
                        r+= to_string(CommRank(world_comm_));
                        fprintf(stderr, "ERROR in %s: Contract not respected, the field \"%s\" is not present in the data model to send. Aborting.\n", r.c_str(), field.name.c_str());
                        MPI_Abort(MPI_COMM_WORLD, 0);
                    }
                }
                else
                {   // The field should not be sent in this iteration, we clear the buffers of the RedistComp
                    //TODO need a clearBuffer on a specific field name
                    data_changed = true;
                    data_filtered->removeData(field.name);
                }
            }
        }
        //else{ // Perform filtering and checking of contractmymy
        for (ContractKey field : list_keys )
        {
            if ( (iteration % field.period) == 0)
            { // This field is sent during this iteration
                if (data->getTypename(field.name, typeName))
                { //Also checks if the field is present in the data model
                    // Performing typechecking
                    if (typeName.compare(field.type) != 0)
                    { //The two types do not match
                        string r = role == DECAF_NODE ? "Node " : "Link ";
                        r+= to_string(CommRank(world_comm_));
                        fprintf(stderr, "ERROR in %s: Contract not respected, sent type %s does not match the type %s of the field \"%s\". Aborting.\n", r.c_str(), typeName.c_str(), field.type.c_str(), field.name.c_str());
                        MPI_Abort(MPI_COMM_WORLD, 0);
                    }
                    data_filtered->appendData(data, field.name); // The field is present of correct type, send it
                }
                else
                {// If the field is not present in data the contract is not respected.
                    string r = role == DECAF_NODE ? "Node " : "Link ";
                    r+= to_string(CommRank(world_comm_));
                    fprintf(stderr, "ERROR in %s: Contract not respected, the field \"%s\" is not present in the data model to send. Aborting.\n", r.c_str(), field.name.c_str());
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
            }
            else // The field should not be sent in this iteration, we clear the buffers of the RedistComp
                //TODO need a clearBuffer on a specific field name
                data_changed = true;
        }
        //}
    }
    else
        return data;


    // If all fields are filtered during this iteration, the message is empty and should not be sent
    if (data_filtered->isEmpty())
    {
        filtered_empty = true;
        return data_filtered;
    }

    // In case there were system fields in the original message
    data_filtered->copySystemFields(data);

    // We attach the current iteration needed for the filtering at the get side or by a link for the put
    if (check_level_ >= CHECK_EVERYWHERE || (role==DECAF_NODE && !no_link_) )
    {
        msgtools::set_iteration(data_filtered, iteration);
    }

    return data_filtered;
}

void
decaf::
Dataflow::filterGet(pConstructData& data, TaskType role){
    if (data->isEmpty() || check_level_ < CHECK_EVERYWHERE){ // If there is no user data or if there is no need to perform the check/filtering
        return;
    }

    if (has_contract() || has_contract_link())
    {
        int it = msgtools::get_iteration(data);
        string typeName;
        vector<ContractKey> &list_keys = (role == DECAF_NODE && has_contract_link()) ? keys_link() : keys() ;
        for (ContractKey field : list_keys )
        {
            if ( (it % field.period) == 0)
            { // This field should be received during this iteration
                if (data->getTypename(field.name, typeName))
                { //Also checks if the field is present in the data model
                    // Performing typechecking
                    if (typeName.compare(field.type) != 0)
                    { //The two types do not match
                        string r = role == DECAF_NODE ? "Node " : "Link ";
                        r+= to_string(CommRank(world_comm_));
                        fprintf(stderr, "ERROR in %s: Contract not respected, sent type %s does not match the type %s of the field \"%s\". Aborting.\n", r.c_str(), typeName.c_str(), field.type.c_str(), field.name.c_str());
                        MPI_Abort(MPI_COMM_WORLD, 0);
                    }
                }
                else
                {// If the field is not present in data the contract is not respected.
                    string r = role == DECAF_NODE ? "Node " : "Link ";
                    r+= to_string(CommRank(world_comm_));
                    fprintf(stderr, "ERROR in %s: Contract not respected, the field \"%s\" is not present in the data model to send. Aborting.\n", r.c_str(), field.name.c_str());
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
            }
            else// The field is not supposed to be received in this iteration
                data->removeData(field.name); //This is silent if the field was not present
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
    else if (role == DECAF_NODE && no_link_)
        redist_prod_con_->clearBuffers();
    else if (role == DECAF_NODE && !no_link_)
        redist_prod_dflow_->clearBuffers();
}
