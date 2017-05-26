//---------------------------------------------------------------------------
// Implement a single feedback mechanism with buffering.
// The consumer signal to the dflow when to send the next iteration
// Signal an error if the buffer overflow
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------


#ifndef DECAF_DATASTREAM_SINGLE_FEEDBACK
#define DECAF_DATASTREAM_SINGLE_FEEDBACK

#include <manala/datastream/datastreaminterface.hpp>
#include <bredala/transport/mpi/channel.hpp>
#include <bredala/transport/redist_comp.h>
#include <bredala/data_model/msgtool.hpp>
#include <manala/storage/storagemainmemory.hpp>
#include <queue>

namespace decaf
{

    class DatastreamSingleFeedback : public Datastream
    {
    public:
        DatastreamSingleFeedback() : Datastream(){}
        DatastreamSingleFeedback(CommHandle world_comm,
               int start_prod,
               int nb_prod,
               int start_dflow,
               int nb_dflow,
               int start_con,
               int nb_con,
               RedistComp* prod_dflow,
               RedistComp* dflow_con,
               ManalaInfo& manala_info);

        DatastreamSingleFeedback(CommHandle world_comm,
                   int start_prod,
                   int nb_prod,
                   int start_con,
                   int nb_con,
                   RedistComp* redist_prod_con,
                   ManalaInfo& manala_info);

        virtual ~DatastreamSingleFeedback();

        virtual void processProd(pConstructData data);

        virtual void processDflow(pConstructData data);

        virtual void processCon(pConstructData data);

    protected:
        unsigned int iteration_;
        OneWayChannel* channel_dflow_;
        OneWayChannel* channel_dflow_con_;
        OneWayChannel* channel_con_;
        bool first_iteration_;
        bool doGet_;
        bool is_blocking_;
    };
} // namespace
#endif


