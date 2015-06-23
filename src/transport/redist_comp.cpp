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

#include "decaf/redist_comp.h"


namespace decaf
{


int decaf::RedistComp::getRankSource()
{
    return rankSource_;
}

int decaf::RedistComp::getNbSources()
{
    return nbSources_;
}

int decaf::RedistComp::getRankDest()
{
    return rankDest_;
}

int decaf::RedistComp::getNbDest()
{
    return nbDests_;
}

void decaf::RedistComp::process(std::shared_ptr<BaseData> data, RedistRole role)
{
    computeGlobal(data, role);

    splitData(data, role);

    redistribute(data, role);
}

} //namespace decaf
