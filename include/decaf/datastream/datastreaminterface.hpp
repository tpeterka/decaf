//---------------------------------------------------------------------------
// Abstract class to define a controlled data stream mechanism.
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_DATASTREAM
#define DECAF_DATASTREAM

#ifdef TRANSPORT_MPI
#include <decaf/transport/mpi/types.h>
#include <decaf/transport/mpi/comm.hpp>
#include <decaf/storage/framemanagersequential.hpp>
#include <decaf/storage/framemanagermostrecent.hpp>
#endif

#include <decaf/storage/storagecollectiongreedy.hpp>
#include <decaf/storage/storagemainmemory.hpp>

namespace decaf
{


    class Datastream
    {
    public:
        Datastream() : initialized_(false){}
        Datastream(CommHandle world_comm,
               int start_prod,
               int nb_prod,
               int start_dflow,
               int nb_dfow,
               int start_con,
               int nb_con,
               RedistComp* prod_dflow,
               RedistComp* dflow_con,
               FramePolicyManagment policy,
               vector<StorageType>& storage_types,
               vector<unsigned int>& max_storage_sizes);
        virtual ~Datastream();

        bool is_prod()          { return world_rank_ >= start_prod_ && world_rank_ < start_prod_ + nb_prod_; }
        bool is_dflow()         { return world_rank_ >= start_dflow_ && world_rank_ < start_dflow_ + nb_dflow_;  }
        bool is_con()           { return world_rank_ >= start_con_ && world_rank_ < start_con_ + nb_con_;  }
        bool is_prod_root()     { return world_rank_ == start_prod_; }
        bool is_dflow_root()    { return world_rank_ == start_dflow_; }
        bool is_con_root()      { return world_rank_ == start_con_; }


        virtual void processProd(pConstructData data) = 0;

        virtual void processDflow(pConstructData data) = 0;

        virtual void processCon(pConstructData) = 0;

    protected:
        bool initialized_;
        CommHandle world_comm_;
        int world_rank_;
        int world_size_;

        int start_prod_;
        int nb_prod_;
        int start_dflow_;
        int nb_dflow_;
        int start_con_;
        int nb_con_;

        CommHandle prod_comm_handle_;
        CommHandle dflow_comm_handle_;
        CommHandle con_comm_handle_;

        RedistComp* redist_prod_dflow_;
        RedistComp* redist_dflow_con_;

        FrameManager* framemanager_;

        StorageCollectionInterface* storage_collection_;

    };
}

decaf::
Datastream::Datastream(CommHandle world_comm,
              int start_prod, int nb_prod,
              int start_dflow, int nb_dflow,
              int start_con, int nb_con,
              RedistComp* redist_prod_dflow,
              RedistComp* redist_dflow_con,
              FramePolicyManagment policy,
              vector<StorageType>& storage_types,
              vector<unsigned int>& max_storage_sizes) :
    initialized_(false), world_comm_(world_comm),
    start_prod_(start_prod), nb_prod_(nb_prod),
    start_dflow_(start_dflow), nb_dflow_(nb_dflow),
    start_con_(start_con), nb_con_(nb_con),
    redist_prod_dflow_(redist_prod_dflow), redist_dflow_con_(redist_dflow_con),
    framemanager_(NULL),storage_collection_(NULL)
{
    MPI_Comm_rank(world_comm_, &world_rank_);
    MPI_Comm_size(world_comm_, &world_size_);

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
                framemanager_ = new FrameManagerRecent(dflow_comm_handle_);
                break;
            }
            case DECAF_FRAME_POLICY_SEQ:
            {
                framemanager_ = new FrameManagerSeq(dflow_comm_handle_);
                break;
            }
            default:
            {
                fprintf(stderr,"WARNING: unrecognized frame policy. Using sequential.\n");
                framemanager_ = new FrameManagerSeq(dflow_comm_handle_);
                break;
            }
        };

        storage_collection_ = new StorageCollectionGreedy();

        for(unsigned int i = 0; i < storage_types.size(); i++)
        {
            switch(storage_types[i])
            {
                case DECAF_STORAGE_MAINMEM:
                {
                    storage_collection_->addBuffer(new StorageMainMemory(max_storage_sizes[i]));
                    break;
                }
                case DECAF_STORAGE_FILE:
                {
                    fprintf(stderr,"DECAF_STORAGE_FILE not implemented yet.\n");
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

decaf::Datastream::~Datastream()
{
    if(framemanager_) delete framemanager_;
}

#endif
