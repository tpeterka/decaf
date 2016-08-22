#include <decaf/transport/mpi/redist_proc_mpi.h>

void
decaf::
RedistProcMPI::computeGlobal(pConstructData& data, RedistRole role)
{
    if( !initialized_ && role == DECAF_REDIST_SOURCE )
    {
        if(nbSources_ % nbDests_ != 0)
        {
            std::cerr<<"ERROR : the number of destination does not divide the number of sources. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        int source_rank;
        MPI_Comm_rank(commSources_, &source_rank);
        destination_ = floor(source_rank / (nbSources_ / nbDests_));
        initialized_ = true;
    }
    if(!initialized_ && role == DECAF_REDIST_DEST )
    {
        if(nbSources_ % nbDests_ != 0)
        {
            std::cerr<<"ERROR : the number of destination does not divide the number of sources. Aborting."<<std::endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        nbReceptions_ = nbSources_ / nbDests_;
        initialized_ = true;
    }

}

void
decaf::
RedistProcMPI::splitData(pConstructData& data, RedistRole role)
{
    // We don't have to do anything here
    data->serialize();
}

void
decaf::
RedistProcMPI::redistribute(pConstructData &data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        if(rank_ == local_dest_rank_ + destination_)
            transit = data;
        else
        {
            MPI_Request req;
            reqs.push_back(req);
            MPI_Isend(data->getOutSerialBuffer(),  data->getOutSerialBufferSize(),
                      MPI_BYTE,  local_dest_rank_ + destination_,
                      send_data_tag, communicator_,&reqs.back());
        }
        send_data_tag = (send_data_tag == INT_MAX ? MPI_DATA_TAG : send_data_tag + 1);
    }

    // check if we have something in transit to/from self
    if (role == DECAF_REDIST_DEST && !transit.empty())
    {
        if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
            data->merge(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
        else if (mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->unserializeAndStore(transit->getOutSerialBuffer(), transit->getOutSerialBufferSize());
        transit.reset();              // we don't need it anymore, clean for the next iteration
        nbReceptions_ = nbSources_ / nbDests_ - 1;
    }

    // only consumer ranks are left
    if (role == DECAF_REDIST_DEST)
    {
        for (int i = 0; i < nbReceptions_; i++)
        {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, recv_data_tag, communicator_, &status);
            int nbytes; // number of bytes in the message
            MPI_Get_count(&status, MPI_BYTE, &nbytes);
            data->allocate_serial_buffer(nbytes); // allocate necessary space
            MPI_Recv(data->getInSerialBuffer(), nbytes, MPI_BYTE, status.MPI_SOURCE,
                     recv_data_tag, communicator_, &status);

            if(mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                data->merge();
            else if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                //data->unserializeAndStore();
                data->unserializeAndStore(data->getInSerialBuffer(), nbytes);
            else
            {
                std::cout<<"ERROR : merge method not specified. Abording."<<std::endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        }
        recv_data_tag = (recv_data_tag == INT_MAX ? MPI_DATA_TAG : recv_data_tag + 1);

        if(mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->mergeStoredData();
    }

}
