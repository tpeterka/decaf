#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <string>
#include <bredala/transport/cci/redist_count_cci.h>
#include <bredala/transport/mpi/redist_count_mpi.h>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/boost_macros.h>
#include <sys/time.h>

using namespace decaf;
using namespace std;

unsigned int nsteps = 5;
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

void run_client(int global_id)
{
    fprintf(stderr, "Running client with the global id %d\n", global_id);

    fprintf(stderr, "Creating the client redistribution component...\n");
    /*RedistCountCCI* redist = new RedistCountCCI(0, 2, 2, 2,
                                                global_id, MPI_COMM_WORLD,
                                                string("testcci.txt"),
                                                DECAF_REDIST_P2P);
    */
    RedistCountMPI* redist = new RedistCountMPI(0, 2, 2, 2, MPI_COMM_WORLD, DECAF_REDIST_P2P);
    fprintf(stderr, "Redistribution component created by the client.\n");

    // TODO: test the case with 0
    srand(1 + global_id);

    for(unsigned int j = 0; j < nsteps; j++)
    {
        fprintf(stderr, "ITERATION %u\n", j);
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

        print_array(my_array, array_size, j);

        delete [] my_array;

        //redist->clearBuffers();
    }
    delete redist;
}

void run_server(int global_id)
{
    fprintf(stderr, "Running server with the global id %d\n", global_id);

    fprintf(stderr, "Creating the client redistribution component...\n");
    /*RedistCountCCI* redist = new RedistCountCCI(0, 2, 2, 2,
                                                global_id, MPI_COMM_WORLD,
                                                string("testcci.txt"),
                                                DECAF_REDIST_P2P);
*/
    RedistCountMPI* redist = new RedistCountMPI(0, 2, 2, 2, MPI_COMM_WORLD, DECAF_REDIST_P2P);
    fprintf(stderr, "Redistribution component created by the server.\n");

    for (unsigned int i = 0; i < nsteps; i++)
    {
        pConstructData result;
        redist->process(result, DECAF_REDIST_DEST);

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
    fprintf(stderr, "usage: %s [-i <id> -s|-c]\n", proc);
    fprintf(stderr, "where:\n");
    fprintf(stderr, "\t-s\tRuns as server\n");
    fprintf(stderr, "\t-c\tRuns as client\n");
    fprintf(stderr, "\t-i\tIndicate the ID of the task\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    MPI_Init(NULL,NULL);

    int size;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    uint32_t caps	= 0;
    int ret = cci_init(CCI_ABI_VERSION, 0, &caps);
    if (ret) {
        fprintf(stderr, "cci_init() failed with %s\n",
            cci_strerror(NULL, (cci_status)ret));
        exit(EXIT_FAILURE);
    }

    char c;
    bool is_server = false, is_client = false;
    int id = -1;

    while ((c = getopt(argc, argv, "i:cs")) != -1) {
        switch (c) {
        case 'c':
            is_client = true;
            break;
        case 's':
            is_server = true;
            break;
        case 'i':
            id = atoi(strdup(optarg));
            break;
        default:
            print_usage(argv[0]);
        }
    }

    if(id == -1 || (!is_client && !is_server))
        print_usage(argv[0]);

    if(is_client)
        run_client(id + rank);
    if(is_server)
        run_server(id + rank);

    cci_finalize();

    MPI_Finalize();

}
