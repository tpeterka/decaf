//---------------------------------------------------------------------------
//
// data interface
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include "bredala/transport/redist_comp.h"
#include <sys/time.h>

// DEPRECATED, for experiments in Bredala paper only
// double timeGlobalRedist = 0.0;


int decaf::RedistComp::getRankSource()
{
    return rankSource_;
}

int
decaf::
RedistComp::getNbSources()
{
    return nbSources_;
}

int
decaf::
RedistComp::getRankDest()
{
    return rankDest_;
}

int
decaf::
RedistComp::getNbDest()
{
    return nbDests_;
}

bool
decaf::
RedistComp::isSource()
{
    return rank_ >= local_source_rank_ && rank_ < local_source_rank_ + nbSources_;
}

bool
decaf::
RedistComp::isDest()
{
    return rank_ >= local_dest_rank_ && rank_ < local_dest_rank_ + nbDests_;
}

// merge the chunks from the vector receivedChunks into one single data object.
decaf::pConstructData
decaf::
RedistComp::merge(RedistRole role)
{
    return pConstructData();
}

// processes the redistribute communication, whether put or get
void
decaf::
RedistComp::process(pConstructData& data,
                    RedistRole role)
{
    if(data->isSystem())
        splitSystemData(data, role);
    else
    {
        computeGlobal(data, role);
        splitData(data, role);
    }

    redistribute(data, role);

}

bool
decaf::
RedistComp::IGet(pConstructData& data)
{
    if(data->isSystem())
        splitSystemData(data, DECAF_REDIST_DEST);
    else
    {
        computeGlobal(data, DECAF_REDIST_DEST);
        splitData(data, DECAF_REDIST_DEST);
    }

    // TODO: replace with asynchronous version
    redistribute(data, DECAF_REDIST_DEST);

    // TODO: change to potentially false when asynchronous
    // redistribution is working
    return true;
}
