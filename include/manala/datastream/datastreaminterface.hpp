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
#include <bredala/transport/mpi/types.h>
#include <bredala/transport/mpi/comm.h>
#include <bredala/transport/redist_comp.h>
#include <manala/selector/framemanagersequential.hpp>
#include <manala/selector/framemanagermostrecent.hpp>
#include <manala/selector/framemanagerlowhigh.hpp>
#endif

#include <manala/storage/storagecollectiongreedy.hpp>
#include <manala/storage/storagecollectionLRU.hpp>
#include <manala/storage/storagemainmemory.hpp>
#include <manala/storage/storagefile.hpp>
#include <manala/types.h>
#include <decaf/types.hpp>

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
               ManalaInfo& manala_info);

        Datastream(CommHandle world_comm,
                   int start_prod,
                   int nb_prod,
                   int start_con,
                   int nb_con,
                   RedistComp* redist_prod_con,
                   ManalaInfo& manala_info);

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

        RedistComp* redist_prod_dflow_; // Only used in the case with link
        RedistComp* redist_dflow_con_;  // Only used in the case with link
        RedistComp* redist_prod_con_;   // Only used in the case without link

        FrameManager* framemanager_;

        StorageCollectionInterface* storage_collection_;

    };
}
#endif
