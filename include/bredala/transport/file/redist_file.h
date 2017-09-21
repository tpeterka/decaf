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

#ifndef DECAF_REDIST_FILE_HPP
#define DECAF_REDIST_FILE_HPP

#include <iostream>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <list>

#include <bredala/transport/redist_comp.h>
#include <bredala/transport/mpi/types.h>

namespace decaf
{


class RedistFile : public RedistComp
{
public:

    RedistFile() :
        task_communicator_(MPI_COMM_NULL){}
    RedistFile(int rankSource,
              int nbSources,
              int rankDest,
              int nbDests,
              int global_rank,
              CommHandle communicator,
              std::string name,
              RedistCommMethod commMethod,
              MergeMethod mergeMethod);
    virtual ~RedistFile();

    virtual void flush();
    void shutdown();

    virtual void clearBuffers();

protected:

    // Compute the values necessary to determine how the data should be
    // splitted and redistributed.
    virtual void computeGlobal(pConstructData& data, RedistRole role) = 0;

    // Seperate the Data into chunks for each destination involve in the
    // component and fill the splitChunks vector
    virtual void splitData(pConstructData& data, RedistRole role) = 0;

    // Seperate system only data model. The data won't be split but duplicated
    void splitSystemData(pConstructData& data, RedistRole role);

    // Transfer the chunks from the sources to the destination. The data should be
    // be stored in the vector receivedChunks
    virtual void redistribute(pConstructData& data, RedistRole role);

    //Redistribution method using only point to point All to All communication
    void redistributeP2P(pConstructData& data, RedistRole role);

    //Redistribution method collective and sending only the necessary messages
    void redistributeCollective(pConstructData& data, RedistRole role);

    CommHandle task_communicator_;      // communicator for all the processes involved in redist
    int task_rank_;                     // Rank of the process within the local task
    int task_size_;                     // Number of processes within the local task
    //std::vector<CommRequest> reqs;    // pending communication requests
    pConstructData transit;             // used when a source and destination are overlapping
    int *sum_;                          // used by the producer
    int *destBuffer_;
    std::string name_;                  // Used as the basename to generate the file names

    std::vector< pConstructData > splitBuffer_;	// Buffer of container to avoid reallocation// used by the consumer
    int64_t send_it;                            // Iteration number of the send
    int64_t get_it;                             // Iteration number of the get
};

} // namespace

#endif
