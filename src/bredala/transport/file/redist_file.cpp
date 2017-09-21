//---------------------------------------------------------------------------
//
// redistribute base class
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <iostream>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <hdf5.h>

#include <bredala/transport/file/redist_file.h>

using namespace std;


// create communicators for contiguous process assignment
// ranks within source and destination must be contiguous, but
// destination ranks need not be higher numbered than source ranks
decaf::
RedistFile::RedistFile(int rankSource,
                     int nbSources,
                     int rankDest,
                     int nbDests,
                     int global_rank,
                     CommHandle communicator,
                     std::string name,
                     RedistCommMethod commMethod,
                     MergeMethod mergeMethod) :
    RedistComp(rankSource, nbSources, rankDest, nbDests, commMethod, mergeMethod),
    task_communicator_(communicator),
    sum_(NULL),
    destBuffer_(NULL),
    name_(name),
    send_it(0),
    get_it(0)
{

    // For File components, we only have the communicator of the local task
    // The rank is the global rank in the application =! from the MPI redist components
    rank_ = global_rank;
    local_dest_rank_ = rankDest;
    local_source_rank_ = rankSource;

    fprintf(stderr, "Global rank: %d\n", rank_);

    MPI_Comm_rank(task_communicator_, &task_rank_);
    MPI_Comm_size(task_communicator_, &task_size_);


    if(isSource() && isDest())
    {
        fprintf(stderr, "ERROR: overlapping case not implemented yet. Abording.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // group with all the sources
    if(isSource())
    {
        // Sanity check
        if(task_size_ != nbSources)
        {
            fprintf(stderr, "ERROR: size of the task communicator (%d) does not match nbSources (%d).\n", task_size_, nbSources);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        // Used only with commMethod = DECAF_REDIST_COLLECTIVE
        summerizeDest_ = new int[ nbDests_];
    }

    // group with all the destinations
    if(isDest())
    {

        // Sanity check
        if(task_size_ != nbDests)
        {
            fprintf(stderr, "ERROR: size of the task communicator (%d) does not match nbDests (%d).\n", task_size_, nbDests);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    // TODO: maybe push that into redist_comp?
    destBuffer_ = new int[nbDests_];    // root consumer side: array sent be the root producer. The array is then scattered by the root consumer
    sum_ = new int[nbDests_];           // root producer side: result array after the reduce of summerizeDest. Member because of the overlapping case: The array need to be accessed by 2 consecutive function calls

    transit = pConstructData(false);

    // Make sure that only the events from the constructor are received during this phase
    // ie the producer cannot start sending data before all the servers created their connections
    MPI_Barrier(task_communicator_);

    // Everyone in the dataflow has read the file, the root clean the uri file
    if (isDest() && task_rank_ == 0)
        std::remove(name_.c_str());
}

decaf::
RedistFile::~RedistFile()
{
    // We don't clean the communicator, it was created by the app
    //if (task_communicator_ != MPI_COMM_NULL)
    //    MPI_Comm_free(&task_communicator_);


    delete[] destBuffer_;
    delete[] sum_;
}

void
decaf::
RedistFile::splitSystemData(pConstructData& data, RedistRole role)
{
    if(role == DECAF_REDIST_SOURCE)
    {
        // Create the array which represents where the current source will emit toward
        // the destinations rank. 0 is no send to that rank, 1 is send
        //if( summerizeDest_) delete [] summerizeDest_;
        //summerizeDest_ = new int[ nbDests_];
        bzero( summerizeDest_,  nbDests_ * sizeof(int)); // First we don't send anything

        // For this case we simply duplicate the message for each destination
        for(unsigned int i = 0; i < nbDests_; i++)
        {
            //Creating the ranges for the split function
            splitChunks_.push_back(data);

            //We send data to everyone except to self
            if(i + local_dest_rank_ != rank_)
                summerizeDest_[i] = 1;
            // rankDest_ - rankSource_ is the rank of the first destination in the
            // component communicator (communicator_)
            destList_.push_back(i + local_dest_rank_);

        }

        // All the data chunks are the same pointer
        // We just need to serialize one chunk
        if(!splitChunks_[0]->serialize())
            std::cout<<"ERROR : unable to serialize one object"<<std::endl;

    }
}

// redistribution protocol
void
decaf::
RedistFile::redistribute(pConstructData& data, RedistRole role)
{
    switch(commMethod_)
    {
        case DECAF_REDIST_COLLECTIVE:
        {
            redistributeCollective(data, role);
            break;
        }
        case DECAF_REDIST_P2P:
        {
            fprintf(stderr, "WARNING: requested P2P redistribution but the method is not implemented yet. Using collective instead.\n");
            redistributeCollective(data, role);
            //redistributeP2P(data, role);
            break;
        }
        default:
        {
            std::cout<<"WARNING : Unknown redistribution strategy used ("<<commMethod_<<"). Using collective by default."<<std::endl;
            redistributeCollective(data, role);
            break;
        }
    }
}

// collective redistribution protocol
void
decaf::
RedistFile::redistributeCollective(pConstructData& data, RedistRole role)
{
    // TODO: write the chunks instead of the data
    if(role == DECAF_REDIST_SOURCE)
    {
        hid_t file_id;
        hid_t plist_id;                 /* property list identifier */

        /*
         * MPI variables
         */
        int mpi_size, mpi_rank;
        MPI_Comm comm  = task_communicator_; // Communicator of the producer only
        MPI_Info info  = MPI_INFO_NULL;
        MPI_Comm_size(comm, &mpi_size);
        MPI_Comm_rank(comm, &mpi_rank);

        /*
         * Set up file access property list with parallel I/O access
         */
         plist_id = H5Pcreate(H5P_FILE_ACCESS);
         H5Pset_fapl_mpio(plist_id, comm, info);

        /*
         * Create a new file collectively and release property list identifier.
         */
        std::stringstream ss;
        ss<<name_<<"_"<<send_it<<".h5.tmp";
        file_id = H5Fcreate(ss.str().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
        H5Pclose(plist_id);

        // Create the local table of sizes
        unsigned int* local_sizes = (unsigned int*)(malloc(nbDests_ * sizeof(unsigned int)));
        bzero(local_sizes, nbDests_ * sizeof(unsigned int));

        // Inverse list of destList
        int* index_table_chunks = (int*)(malloc(nbDests_ * sizeof(unsigned int)));
        unsigned int index_split_chunks = 0;

        // TODO Matthieu
        // Organisation of the destList, summerizeDest and ChunkList is very confusing
        // Need to reorganize the way the split and redistribute functions are working together
        for(unsigned int i = 0; i < nbDests_; i++)
        {
            if(summerizeDest_[i] == 1)
            {
                local_sizes[i] = splitChunks_[index_split_chunks]->getOutSerialBufferSize();
                index_table_chunks[i] = index_split_chunks;
                index_split_chunks++;
            }
        }

        // Aggregate the global table of sizes
        unsigned int* global_sizes = (unsigned int*)(malloc(nbDests_ * nbSources_ * sizeof(unsigned int)));
        MPI_Allgather(local_sizes, nbDests_, MPI_UNSIGNED, global_sizes, nbDests_ , MPI_UNSIGNED, task_communicator_);

        // For each data model, creating a dataspace and writing the data collectively
        for(unsigned int i = 0; i < nbDests_ * nbSources_; i++)
        {
            // No data to send for this destination
            if(global_sizes[i] == 0)
                continue;

            hid_t filespace, memspace, dset_id, dataset_prod_id;
            hsize_t	count[2];	          /* hyperslab selection parameters */
            hsize_t	stride[2];
            hsize_t	block[2];
            hsize_t	offset[2];
            hsize_t dims[1];
            herr_t status;

            // Individual size of the dataset, chunk and block
            dims[0] = global_sizes[i];

            // Creating the dataspace
            filespace = H5Screate_simple(1, dims, NULL); // Number of dimensions in the dataspace, size of each dimension

            // Creating the memspace
            memspace = H5Screate_simple(1, dims, NULL);

            // Creating the name of the dataset
            unsigned int source_id = i / nbDests_;
            unsigned int dest_id = i % nbDests_;
            std::stringstream datasetname;
            datasetname<<"Container_"<<source_id<<"_"<<dest_id;

            // Creating chunked dataset
            //dset_id = H5Dcreate(file_id, datasetname.str().c_str(), H5T_NATIVE_CHAR, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            plist_id = H5Pcreate(H5P_DATASET_CREATE);
            H5Pset_chunk(plist_id, 1, dims);

            dset_id = H5Dcreate(file_id, datasetname.str().c_str(), H5T_C_S1, filespace,
                        H5P_DEFAULT, plist_id, H5P_DEFAULT);
            H5Pclose(plist_id);
            H5Sclose(filespace);

            /*
             * Select hyperslab in the file.
             */
            // Probably should use only 1D arrays for count, stride and block
            if(source_id == mpi_rank) // If we have the actual data
            {
                count[0] = 1;
                count[1] = 1 ;
                stride[0] = 1;
                stride[1] = 1;
                block[0] = global_sizes[i];
                block[1] = global_sizes[i];
            }
            else
            {
                count[0] = 1;
                count[1] = 1 ;
                stride[0] = 1;
                stride[1] = 1;
                block[0] = 0;
                block[1] = 0;
            }

            filespace = H5Dget_space(dset_id);
            status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

            // Create the property list
            dataset_prod_id = H5Pcreate(H5P_DATASET_XFER);
            //H5Pset_dxpl_mpio(dataset_prod_id, H5FD_MPIO_COLLECTIVE);
            H5Pset_dxpl_mpio(dataset_prod_id, H5FD_MPIO_INDEPENDENT);

            // Write the dataset
            if(source_id == mpi_rank) // If we have the actual data
            {
                status = H5Dwrite(dset_id, H5T_C_S1, H5S_ALL, H5S_ALL, dataset_prod_id, splitChunks_[index_table_chunks[dest_id]]->getOutSerialBuffer());
            }

            H5Dclose(dset_id);
            H5Sclose(filespace);
            H5Pclose(dataset_prod_id);
        }

        // Cleaning memory
        free(local_sizes);
        free(global_sizes);
        free(index_table_chunks);

        // Close the file
        H5Fclose(file_id);

        // Waiting that everyone write its data then changing the filename
        MPI_Barrier(task_communicator_);
        if (task_rank_ == 0)
        {
            std::stringstream filename;
            filename<<name_<<"_"<<send_it<<".h5";
            rename(ss.str().c_str(), filename.str().c_str());
        }
        send_it++;
    }

    // Make sure that we process all the requests of 1 iteration before receiving events from the next one
    // TODO: use the queue event to manage stored events belonging to other iterations
    // NOTE Matthieu: Implemented poll_event. Have to check on a cluster to see if it holds.
    //MPI_Barrier(task_communicator_);
    if (role == DECAF_REDIST_DEST)
    {
        /*
         * MPI variables
         */
        int mpi_size, mpi_rank;
        MPI_Comm comm  = task_communicator_; // Communicator of the producer only
        MPI_Comm_size(comm, &mpi_size);
        MPI_Comm_rank(comm, &mpi_rank);

        hid_t file_id, dset_id, dspace_id;

        /*
         * Create a new file collectively and release property list identifier.
         */
        std::stringstream ss;
        ss<<name_<<"_"<<get_it<<".h5";

        // Checking that the file exist
        struct stat buffer;
        while(stat(ss.str().c_str(), & buffer) != 0)
            usleep(100000); // 100ms

        // Opening the file
        file_id = H5Fopen(ss.str().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

        // Reading all the dataset for this destination, one per source
        for (unsigned int i = 0; i < nbSources_; i++)
        {
            // Creating the name of the dataset
            std::stringstream datasetname;
            datasetname<<"Container_"<<i<<"_"<<task_rank_;

            // Checking if the dataset exist
            htri_t check = H5Lexists(file_id, datasetname.str().c_str(), H5P_DEFAULT);
            if(check == 0)
                continue;

            // Opening the dataset
            dset_id = H5Dopen(file_id, datasetname.str().c_str(), H5P_DEFAULT);

            // Getting the size information of the dataset
            dspace_id = H5Dget_space(dset_id);
            hsize_t dims[1]; // We only have 1D arrays
            H5Sget_simple_extent_dims(dspace_id, dims, NULL);

            // Reading the dataset
            data->allocate_serial_buffer(dims[0]);
            herr_t status = H5Dread(dset_id, H5T_C_S1, H5S_ALL, H5S_ALL, H5P_DEFAULT, data->getInSerialBuffer());

            if (mergeMethod_ == DECAF_REDIST_MERGE_STEP)
                data->merge();
            else if (mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
                //data->unserializeAndStore();
                data->unserializeAndStore(data->getInSerialBuffer(), dims[0]);
            else
            {
                fprintf(stderr, "ERROR: merge method not specified. Aborting.\n");
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        }

        if (mergeMethod_ == DECAF_REDIST_MERGE_ONCE)
            data->mergeStoredData();

        // TODO: destroy the file
        get_it++;

    }
}

// point to point redistribution protocol
void
decaf::
RedistFile::redistributeP2P(pConstructData& data, RedistRole role)
{
    fprintf(stderr, "Redistribution by file not implemented yet.");

    // Make sure that we process all the requests of 1 iteration before receiving events from the next one
    // TODO: use the queue event to manage stored events belonging to other iterations
    // NOTE Matthieu: Implemented poll_event. Have to check on a cluster to see if it holds.
    //MPI_Barrier(task_communicator_);
}

void
decaf::
RedistFile::flush()
{
    /*if (reqs.size())
        MPI_Waitall(reqs.size(), &reqs[0], MPI_STATUSES_IGNORE);
    reqs.clear();*/

    // data have been received, we can clean it now
    // Cleaning the data here because synchronous send.
    // TODO: move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

void
decaf::
RedistFile::shutdown()
{
    /*for (size_t i = 0; i < reqs.size(); i++)
        MPI_Request_free(&reqs[i]);
    reqs.clear();*/

    // data have been received, we can clean it now
    // Cleaning the data here because synchronous send.
    // TODO: move to flush when switching to asynchronous send
    splitChunks_.clear();
    destList_.clear();
}

void
decaf::
RedistFile::clearBuffers()
{
    splitBuffer_.clear();
}

