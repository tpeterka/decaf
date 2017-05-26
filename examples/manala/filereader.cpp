//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example with buffering mechanism
//
// prod (1 procs) - con (1 procs)
//
// entire workflow takes 3 procs (1 dataflow procs between prod and con)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <unistd.h>

using namespace decaf;
using namespace std;

// producer
void filereader(Decaf* decaf, string filebase, int start_rank_file, int nb_files)
{
    // Checking if we can distribute the files properly
    int comm_size = decaf->local_comm_size();
    int local_rank = decaf->local_comm_rank();

    if(nb_files < comm_size)
    {
        fprintf(stderr, "ERROR: more reader than meta files. Use a number of reader diving the number of meta files.\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if(nb_files % comm_size != 0)
    {
        fprintf(stderr, "ERROR: The number of reader does not divide the number of meta files.\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    int nb_files_per_rank = nb_files / comm_size;
    int local_first_rank_file = start_rank_file + local_rank * nb_files_per_rank;

    // Creating the list of files for each meta files
    vector<vector<pair<unsigned int,string> > >filelist(nb_files_per_rank);

    // Reading the metafiles and filling the file list
    for(int i = 0; i < nb_files_per_rank; i++)
    {
        stringstream filename;
        filename<<filebase<<i+local_first_rank_file<<".txt";

        ifstream file(filename.str());
        if(!file.good())
        {
            fprintf(stderr, "ERROR: unable to load the file %s. Wrong filebase?\n", filename.str().c_str());
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        while(true)
        {
            unsigned int id;
            string savename;
            file>>id>>savename;
            if(file.eof()) break;
            fprintf(stderr, "New save for ID %u: %s\n", id, savename.c_str());
            filelist[i].emplace_back(id, savename);
        }
    }

    // Sanity check
    // Checking the all the metadata files have the same number of saved files
    unsigned int nb_iterations = filelist[0].size();
    for(int i = 1; i < nb_files_per_rank - 1; i++)
    {
        if(filelist[i].size() != nb_iterations)
        {
            fprintf(stderr,"ERROR: the metafiles don't have the same number of saved files.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    // Checking that all the metadata files have the same id of saved files
    for(unsigned int i = 0; i < nb_iterations; i++)
    {
        unsigned int id = filelist[0][i].first;
        for(unsigned int j = 1; j < filelist.size(); j++)
        {
            if(filelist[j][i].first != id)
            {
                fprintf(stderr,"ERROR: the metadata files don't have the same iterations saved.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
        }
    }

    // Everything is clean, we can create the storage and start to load data
    vector<StorageFile> stores;
    for(int i = 0; i < nb_files_per_rank; i++)
    {
        stores.emplace_back(nb_iterations, 0); // No need for valid parameters, we won't write
        stores.back().initFromFilelist(filelist[i]);
    }

    // Running the loop forwarding the data to the rest of the application.
    for(unsigned int i = 0; i < nb_iterations; i++)
    {
        unsigned int id = stores[0].getID(i);
        pConstructData data = stores[0].getData(id);

        // Merging all the data stored with the same ID
        for(unsigned int j = 1; j < stores.size(); j++)
        {
            pConstructData other = stores[j].getData(id);
            data->merge(other.getPtr());
        }

        decaf->put(data);
    }

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "reader terminating\n");
    decaf->terminate();
}

int main(int argc,
         char** argv)
{
    if(argc != 4)
    {
        fprintf(stderr, "Usage: filereader filenamebase start_rank nb_rank\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    string filebase = string(argv[1]);
    int start_rank = atoi(argv[2]);
    int nb_rank = atoi(argv[3]);

    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "buffer.json");

    MPI_Init(NULL, NULL);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    // run decaf
    filereader(decaf, filebase, start_rank, nb_rank);

    // cleanup
    delete decaf;
    MPI_Finalize();

    return 0;
}
