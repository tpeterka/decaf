#include <bredala/transport/file/redist_proc_file.h>

void
decaf::
RedistProcFile::computeGlobal(pConstructData& data, RedistRole role)
{
    if( !initializedSource_ && role == DECAF_REDIST_SOURCE )
    {
        if(nbSources_ > nbDests_ && nbSources_ % nbDests_ != 0)
        {
            std::cerr<<"ERROR : the number of destination does not divide the number of sources. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        else if(nbSources_ < nbDests_ && nbDests_ % nbSources_ != 0)
        {
            std::cerr<<"ERROR : the number of sources does not divide the number of destinations. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        int source_rank;
        MPI_Comm_rank( task_communicator_, &source_rank);

        if(nbSources_ >= nbDests_) //N to M case (gather)
        {
            destination_ = floor(source_rank / (nbSources_ / nbDests_));
            nbSends_ = 1;
        }
        else // M to N case (broadcast)
        {
            nbSends_ = nbDests_ / nbSources_;
            destination_ = source_rank * nbSends_;
        }
        initializedSource_ = true;
    }
    if(!initializedRecep_ && role == DECAF_REDIST_DEST )
    {
        if(nbSources_ > nbDests_ && nbSources_ % nbDests_ != 0)
        {
            std::cerr<<"ERROR : the number of destination does not divide the number of sources. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        else if(nbSources_ < nbDests_ && nbDests_ % nbSources_ != 0)
        {
            std::cerr<<"ERROR : the number of sources does not divide the number of destinations. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        if(nbSources_ > nbDests_)
            nbReceptions_ = nbSources_ / nbDests_;
        else
            nbReceptions_ = 1;
        initializedRecep_ = true;
    }

}

void
decaf::
RedistProcFile::splitData(pConstructData& data, RedistRole role)
{
    // We don't have to do anything here
    data->serialize();
}

// point to point redistribution protocol
void
decaf::
RedistProcFile::redistribute(pConstructData& data, RedistRole role)
{
    fprintf(stderr, "ERROR: Proc redistribution by file not implemented yet.\n");

    // Make sure that we process all the requests of 1 iteration before receiving events from the next one
    // TODO: use the queue event to manage stored events belonging to other iterations
    // NOTE Matthieu: Implemented poll_event. Have to check on a cluster to see if it holds.
    //MPI_Barrier(task_communicator_);
}
