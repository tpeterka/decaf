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

#include "decaf/transport/redist_comp.h"

int
decaf::
RedistComp::getRankSource()
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
std::shared_ptr<decaf::BaseData>
decaf::
RedistComp::merge(RedistRole role)
{
    return std::shared_ptr<BaseData>();
}

// processes the redistribute communication, whether put or get
// returns 0: in the case of get, nothing was received
//         1: in the case of get, something was received
// the return value can be ignored in the case of put
int
decaf::
RedistComp::process(std::shared_ptr<BaseData> data,
                    RedistRole role)
{
    computeGlobal(data, role);
    splitData(data, role);
    return redistribute(data, role);
}
