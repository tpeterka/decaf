#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <string>
#include <bredala/transport/file/redist_round_file.h>
#include <bredala/transport/mpi/redist_round_mpi.h>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/boost_macros.h>
#include <sys/time.h>

using namespace decaf;
using namespace std;

unsigned int max_array_size = 10;

void print_array(unsigned int* array, unsigned int size, unsigned int it)
{
    stringstream ss;
    ss<<"It "<<it<<"[";
    for(unsigned int i = 0; i < size; i++)
        ss<<array[i]<<",";
    ss<<"]";
    fprintf(stderr, "%s\n", ss.str().c_str());
}

void run_client(int nb_client, int nb_server, int nb_it, bool use_mpi)
{
    int size;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int global_id = rank;
    fprintf(stderr, "Running client with the global id %d\n", global_id);

    RedistComp* redist;
    if(use_mpi)
        redist = new RedistRoundMPI(0, nb_client, nb_client, nb_server, MPI_COMM_WORLD, DECAF_REDIST_P2P);
    else
        redist = new RedistRoundFile(0, nb_client, nb_client, nb_server,
                                                global_id, MPI_COMM_WORLD,
                                                string("testfile.txt"),
                                                DECAF_REDIST_COLLECTIVE);

    // TODO: test the case with 0
    srand(1 + global_id);

    for(unsigned int j = 0; j < nb_it; j++)
    {
        pConstructData container;

        unsigned int array_size = rand() % max_array_size;

        unsigned int* my_array = new unsigned int[array_size];
        ArrayFieldu my_array_field(my_array, array_size, 1, false);

        for (unsigned int i = 0; i < array_size; i++)
            my_array[i] = global_id * max_array_size + i + j * 100;

        container->appendData("my_array", my_array_field,
                              DECAF_NOFLAG,DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

        redist->process(container, DECAF_REDIST_SOURCE);
        redist->flush();

        print_array(my_array, array_size, j);

        delete [] my_array;

        //redist->clearBuffers();
    }
    delete redist;
}

void run_server(int nb_client, int nb_server, int nb_it, bool use_mpi)
{
    int size;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int global_id = nb_client + rank;

    fprintf(stderr, "Running server with the global id %d\n", global_id);

    RedistComp* redist;
    if(use_mpi)
        redist = new RedistRoundMPI(0, nb_client, nb_client, nb_server, MPI_COMM_WORLD, DECAF_REDIST_P2P);
    else
        redist = new RedistRoundFile(0, nb_client, nb_client, nb_server,
                                                global_id, MPI_COMM_WORLD,
                                                string("testfile.txt"),
                                                DECAF_REDIST_COLLECTIVE);


    for (unsigned int i = 0; i < nb_it; i++)
    {
        pConstructData result;
        redist->process(result, DECAF_REDIST_DEST);
        redist->flush();

        ArrayFieldu my_array_field = result->getFieldData<ArrayFieldu>("my_array");
        if(!my_array_field)
            print_array(nullptr, 0, i);
        else
        {
            unsigned int* array = my_array_field.getArray();
            unsigned int size = my_array_field.getArraySize();
            print_array(array, size, i);
        }

        //redist->clearBuffers();

        sleep(1);
    }

    delete redist;
}

static void print_usage(char *proc)
{
    fprintf(stderr, "usage: %s [-n <nb_client> -m <nb_server> -s|-c][-i <nsteps>][-p]\n", proc);
    fprintf(stderr, "where:\n");
    fprintf(stderr, "\t-s\tRuns as server\n");
    fprintf(stderr, "\t-c\tRuns as client\n");
    fprintf(stderr, "\t-n\tIndicate the number of clients\n");
    fprintf(stderr, "\t-m\tIndicate the number of servers\n");
    fprintf(stderr, "\t-i\tIndicate the number of iterations to redistribute\n");
    fprintf(stderr, "\t-p\tUse MPI redistribution instead of CCI.\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    MPI_Init(NULL,NULL);

    char c;
    bool is_server = false, is_client = false;
    int nb_client = -1;
    int nb_server = -1;
    int nb_it = 5;
    bool use_mpi = false;

    while ((c = getopt(argc, argv, "n:m:csi:p")) != -1)
    {
        switch (c) {
        case 'c':
            is_client = true;
            break;
        case 's':
            is_server = true;
            break;
        case 'n':
            nb_client = atoi(strdup(optarg));
            break;
        case 'm':
            nb_server = atoi(strdup(optarg));
            break;
        case 'i':
            nb_it = atoi(strdup(optarg));
            break;
        case 'p':
            use_mpi = true;
            break;
        default:
            print_usage(argv[0]);
        }
    }

    if(nb_client == -1 || nb_server == -1 || nb_it < 1 || (!is_client && !is_server))
        print_usage(argv[0]);

    if(use_mpi)
        fprintf(stderr, "Using the MPI redistribution component.\n");
    else
        fprintf(stderr, "Using the File redistribution component.\n");

    if(is_client)
        run_client(nb_client, nb_server, nb_it, use_mpi);
    if(is_server)
        run_server(nb_client, nb_server, nb_it, use_mpi);

    MPI_Finalize();

}
