#include <manala/datastream/datastreaminterface.hpp>
#include <manala/tools.h>

decaf::
Datastream::Datastream(CommHandle world_comm,
              int start_prod, int nb_prod,
              int start_dflow, int nb_dflow,
              int start_con, int nb_con,
              RedistComp* redist_prod_dflow,
              RedistComp* redist_dflow_con,
              ManalaInfo& manala_info) :
    initialized_(false), world_comm_(world_comm),
    start_prod_(start_prod), nb_prod_(nb_prod),
    start_dflow_(start_dflow), nb_dflow_(nb_dflow),
    start_con_(start_con), nb_con_(nb_con),
    redist_prod_dflow_(redist_prod_dflow), redist_dflow_con_(redist_dflow_con),
    framemanager_(NULL),storage_collection_(NULL)
{
    MPI_Comm_rank(world_comm_, &world_rank_);
    MPI_Comm_size(world_comm_, &world_size_);

    FramePolicyManagment policy = stringToFramePolicyManagment( manala_info.frame_policy );

    // Creation of the channels
    if(is_prod())
    {

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

        switch(policy)
        {
            case DECAF_FRAME_POLICY_RECENT:
            {
                framemanager_ = new FrameManagerRecent(MPI_COMM_NULL, DECAF_NODE, manala_info.prod_freq_output);
                break;
            }
            case DECAF_FRAME_POLICY_SEQ:
            {
                framemanager_ = new FrameManagerSeq(MPI_COMM_NULL, DECAF_NODE, manala_info.prod_freq_output);
                break;
            }
            case DECAF_FRAME_POLICY_LOWHIGH:
            {
                framemanager_ = new FrameManagerLowHigh(MPI_COMM_NULL, DECAF_NODE, manala_info.low_frequency, manala_info.high_frequency);
                break;
            }
            default:
            {
                fprintf(stderr,"WARNING: unrecognized frame policy. Using sequential.\n");
                framemanager_ = new FrameManagerSeq(MPI_COMM_NULL, DECAF_NODE, manala_info.prod_freq_output);
                break;
            }
        };
    }

    if(is_dflow())
    {
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

        //framemanager_ = new FrameManagerSeq(dflow_comm_handle_);
        switch(policy)
        {
            case DECAF_FRAME_POLICY_RECENT:
            {
                framemanager_ = new FrameManagerRecent(dflow_comm_handle_, DECAF_LINK, manala_info.prod_freq_output);
                break;
            }
            case DECAF_FRAME_POLICY_SEQ:
            {
                framemanager_ = new FrameManagerSeq(dflow_comm_handle_, DECAF_LINK, manala_info.prod_freq_output);
                break;
            }
            case DECAF_FRAME_POLICY_LOWHIGH:
            {
                framemanager_ = new FrameManagerLowHigh(dflow_comm_handle_, DECAF_LINK, manala_info.low_frequency, manala_info.high_frequency);
                break;
            }
            default:
            {
                fprintf(stderr,"WARNING: unrecognized frame policy. Using sequential.\n");
                framemanager_ = new FrameManagerSeq(dflow_comm_handle_, DECAF_LINK, manala_info.prod_freq_output);
                break;
            }
        };

        StorageCollectionPolicy storage_policy = stringToStorageCollectionPolicy(manala_info.storage_policy);

        switch(storage_policy)
        {
            case DECAF_STORAGE_COLLECTION_LRU:
            {
                storage_collection_ = new StorageCollectionLRU();
                break;
            }
            case DECAF_STORAGE_COLLECTION_GREEDY:
            {
                storage_collection_ = new StorageCollectionGreedy();
                break;
            }
            default:
                storage_collection_ = new StorageCollectionGreedy();
        }



        for(unsigned int i = 0; i < manala_info.storages.size(); i++)
        {
            switch(manala_info.storages[i])
            {
                case DECAF_STORAGE_MAINMEM:
                {
                    storage_collection_->addBuffer(new StorageMainMemory(manala_info.storage_max_buffer[i]));
                    break;
                }
                case DECAF_STORAGE_FILE:
                {
                    storage_collection_->addBuffer(new StorageFile(manala_info.storage_max_buffer[i], world_rank_));
                    break;
                }
                case DECAF_STORAGE_DATASPACE:
                {
                    fprintf(stderr,"DECAF_STORAGE_DATASPACE not implemented yet.\n");
                    break;
                }
                default:
                {
                    fprintf(stderr, "Unknown storage type. Skiping.\n");
                    break;
                }
            };
        }

        if(storage_collection_->getNbStorageObjects() == 0)
        {
            fprintf(stderr, "ERROR: Using streams but no storage objects were created successfully.\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

    }

    if(is_con())
    {
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
}

decaf::
Datastream::Datastream(CommHandle world_comm,
           int start_prod,
           int nb_prod,
           int start_con,
           int nb_con,
           RedistComp* redist_prod_con,
           ManalaInfo& manala_info) :
    initialized_(false), world_comm_(world_comm),
    start_prod_(start_prod), nb_prod_(nb_prod),
    start_con_(start_con), nb_con_(nb_con),
    redist_prod_con_(redist_prod_con),
    framemanager_(NULL),storage_collection_(NULL)
{
    MPI_Comm_rank(world_comm_, &world_rank_);
    MPI_Comm_size(world_comm_, &world_size_);

    FramePolicyManagment policy = stringToFramePolicyManagment( manala_info.frame_policy );

    // Creation of the channels
    if(is_prod())
    {

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

        switch(policy)
        {
            case DECAF_FRAME_POLICY_RECENT:
            {
                framemanager_ = new FrameManagerRecent(MPI_COMM_NULL, DECAF_NODE, manala_info.prod_freq_output);
                break;
            }
            case DECAF_FRAME_POLICY_SEQ:
            {
                framemanager_ = new FrameManagerSeq(MPI_COMM_NULL, DECAF_NODE, manala_info.prod_freq_output);
                break;
            }
            default:
            {
                fprintf(stderr,"WARNING: unrecognized frame policy. Using sequential.\n");
                framemanager_ = new FrameManagerSeq(MPI_COMM_NULL, DECAF_NODE, manala_info.prod_freq_output);
                break;
            }
        };
    }

    if(is_con())
    {
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
}

decaf::Datastream::~Datastream()
{
    if(framemanager_) delete framemanager_;
}
