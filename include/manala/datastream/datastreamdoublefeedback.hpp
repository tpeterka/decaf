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

#include <manala/datastream/datastreaminterface.hpp>
//#include <decaf/transport/mpi/channel.hpp>
#include <decaf/transport/redist_comp.h>
#include <decaf/data_model/msgtool.hpp>
#include <manala/storage/storagemainmemory.hpp>
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
               RedistComp* dflow_con,
               ManalaInfo& manala_info);

        DatastreamDoubleFeedback(CommHandle world_comm,
                   int start_prod,
                   int nb_prod,
                   int start_con,
                   int nb_con,
                   RedistComp* redist_prod_con,
                   ManalaInfo& manala_info);

        virtual ~DatastreamDoubleFeedback();

        virtual void processProd(pConstructData data);

        virtual void processDflow(pConstructData data);

        virtual void processCon(pConstructData data);

    protected:
        unsigned int iteration_;
        OneWayChannel* channel_prod_;
        OneWayChannel* channel_prod_dflow_;
        OneWayChannel* channel_dflow_prod_;
        OneWayChannel* channel_dflow_;
        OneWayChannel* channel_dflow_con_;
        OneWayChannel* channel_con_;
        bool first_iteration_;
        bool doGet_;
        bool is_blocking_;
    };
} // namespace
#endif


