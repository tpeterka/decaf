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
#include <sys/time.h>

double timeGlobalRedist = 0.0;

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
    double timeGlobal = 0.0, timeSplit = 0.0, timeRedist = 0.0;

    struct timeval begin;
    struct timeval end;

    gettimeofday(&begin, NULL);
    computeGlobal(data, role);
    gettimeofday(&end, NULL);
    timeGlobal = end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
    
    gettimeofday(&begin, NULL);
    splitData(data, role);
    gettimeofday(&end, NULL);
    timeSplit = end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);

    gettimeofday(&begin, NULL);
    //redistribute(data, role);
    gettimeofday(&end, NULL);
    timeRedist = end.tv_sec+(end.tv_usec/1000000.0) - begin.tv_sec - (begin.tv_usec/1000000.0);
    //if(role == DECAF_REDIST_SOURCE)
    //    printf(" [SOURCE] Global : %f, Split : %f, Redist : %f\n", timeGlobal, timeSplit, timeRedist);
    //if(role == DECAF_REDIST_DEST)
    //    printf(" [DEST] Global : %f, Split : %f, Redist : %f\n", timeGlobal, timeSplit, timeRedist);
    timeGlobalRedist += timeRedist;

}

} //namespace decaf
