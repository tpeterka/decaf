#include <bredala/transport/file/redist_proc_file.h>
#include <bredala/data_model/arrayfield.hpp>
#include <hdf5.h>
#include <sys/stat.h>

void print_array_debug(unsigned int* array, unsigned int size, unsigned int it)
{
    std::stringstream ss;
    ss<<"[DEBUG] It "<<it<<"[";
    for(unsigned int i = 0; i < size; i++)
        ss<<array[i]<<",";
    ss<<"]";
    fprintf(stderr, "%s\n", ss.str().c_str());
}

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

        // Gather case
        if(nbSources_ > nbDests_)
        {
            nbReceptions_ = nbSources_ / nbDests_;
            startReception_ = task_rank_ * nbReceptions_;
            fprintf(stderr, "Will read from container %u to %u\n", startReception_, startReception_ + nbReceptions_);
        }
        else // Broadcast case
        {
            nbReceptions_ = 1;
            int destPerSource = nbDests_ / nbSources_;
            startReception_ = task_rank_ / destPerSource;
            fprintf(stderr, "Will read from container %u to %u\n", startReception_, startReception_ + nbReceptions_);
        }
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
        // Each producer only write once its data
        // the readers will select with dataset to read
        unsigned int* local_sizes = (unsigned int*)(malloc(sizeof(unsigned int)));
        local_sizes[0] = data->getOutSerialBufferSize();

        // Aggregate the global table of sizes
        unsigned int* global_sizes = (unsigned int*)(malloc(nbSources_ * sizeof(unsigned int)));
        MPI_Allgather(local_sizes, 1, MPI_UNSIGNED, global_sizes, 1 , MPI_UNSIGNED, task_communicator_);

        // For each data model, creating a dataspace and writing the data collectively
        for(unsigned int i = 0; i < nbSources_; i++)
        {
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
            unsigned int source_id = i;
            std::stringstream datasetname;
            datasetname<<"Container_"<<source_id;

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
                status = H5Dwrite(dset_id, H5T_C_S1, H5S_ALL, H5S_ALL, dataset_prod_id, data->getOutSerialBuffer());
                fprintf(stderr, "Writing a dataset of size %u, global size: %u\n", data->getOutSerialBufferSize(), global_sizes[i]);
            }

            H5Dclose(dset_id);
            H5Sclose(filespace);
            H5Pclose(dataset_prod_id);
        }

        // Cleaning memory
        free(local_sizes);
        free(global_sizes);

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
        for (unsigned int i = startReception_; i < startReception_ + nbReceptions_; i++)
        {
            // Creating the name of the dataset
            std::stringstream datasetname;
            datasetname<<"Container_"<<i;

            // Opening the dataset
            dset_id = H5Dopen(file_id, datasetname.str().c_str(), H5P_DEFAULT);

            // Getting the size information of the dataset
            dspace_id = H5Dget_space(dset_id);
            hsize_t dims[1]; // We only have 1D arrays
            H5Sget_simple_extent_dims(dspace_id, dims, NULL);
            fprintf(stderr, "[%d]: Reading dataset \"%s\", size %llu\n", task_rank_, datasetname.str().c_str(), dims[0]);

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
