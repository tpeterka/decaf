#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <string>
#include <bredala/transport/cci/redist_count_cci.h>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/boost_macros.h>

using namespace decaf;
using namespace std;

void print_array(unsigned int* array, unsigned int size)
{
    stringstream ss;
    ss<<"[";
    for(unsigned int i = 0; i < size; i++)
        ss<<array[i]<<",";
    ss<<"]";
    fprintf(stderr, "%s\n", ss.str().c_str());
}

void run_client(int global_id)
{
    fprintf(stderr, "Running client with the global id %d\n", global_id);

    fprintf(stderr, "Creating the client redistribution component...\n");
    RedistCountCCI* redist = new RedistCountCCI(0, 2, 2, 2,
                                                global_id, MPI_COMM_WORLD,
                                                string("testcci.txt"),
                                                DECAF_REDIST_P2P);
    fprintf(stderr, "Redistribution component created by the client.\n");

    pConstructData container;

    unsigned int array_size = 10;

    unsigned int* my_array = new unsigned int[array_size];
    ArrayFieldu my_array_field(my_array, 10, 1, false);

    for (unsigned int i = 0; i < array_size; i++)
        my_array[i] = global_id * array_size + i;

    container->appendData("my_array", my_array_field,
                          DECAF_NOFLAG,DECAF_PRIVATE,
                          DECAF_SPLIT_DEFAULT, DECAF_MERGE_DEFAULT);

    fprintf(stderr,"Processing data on the client side...\n");
    redist->process(container, DECAF_REDIST_SOURCE);
    fprintf(stderr, "Data processed on the client side.\n");
    print_array(my_array, array_size);

    delete [] my_array;
    delete redist;
}

void run_server(int global_id)
{
    fprintf(stderr, "Running server with the global id %d\n", global_id);

    fprintf(stderr, "Creating the client redistribution component...\n");
    RedistCountCCI* redist = new RedistCountCCI(0, 2, 2, 2,
                                                global_id, MPI_COMM_WORLD,
                                                string("testcci.txt"),
                                                DECAF_REDIST_P2P);
    fprintf(stderr, "Redistribution component created by the server.\n");

    pConstructData result;
    fprintf(stderr,"Processing data on the server side...\n");
    redist->process(result, DECAF_REDIST_DEST);
    fprintf(stderr, "Data processed on the server side.\n");

    ArrayFieldu my_array_field = result->getFieldData<ArrayFieldu>("my_array");
    if(!my_array_field)
        fprintf(stderr, "ERROR: field my_array not found.\n");

    unsigned int* array = my_array_field.getArray();
    unsigned int size = my_array_field.getArraySize();
    print_array(array, size);

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
