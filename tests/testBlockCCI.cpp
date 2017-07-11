#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <string>
#include <sstream>
#include <bredala/transport/cci/redist_block_cci.h>
#include <bredala/transport/mpi/redist_block_mpi.h>
#include <bredala/data_model/pconstructtype.h>
#include <bredala/data_model/arrayfield.hpp>
#include <bredala/data_model/blockfield.hpp>
#include <bredala/data_model/simplefield.hpp>
#include <bredala/data_model/boost_macros.h>

#include "tools.hpp"
#include <sys/time.h>

using namespace decaf;
using namespace std;


string filebasename = string("block_cci");

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
        redist = new RedistBlockMPI(0, nb_client, nb_client, nb_server, MPI_COMM_WORLD, DECAF_REDIST_P2P);
    else
        redist = new RedistBlockCCI(0, nb_client, nb_client, nb_server,
                                                global_id, MPI_COMM_WORLD,
                                                string("testcci.txt"),
                                                DECAF_REDIST_P2P);

    vector<float> bbox = {0.0,0.0,0.0,10.0,10.0,10.0};

    // TODO: test the case with 0
    srand(1 + global_id);
    unsigned int r,g,b;
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;

    for(unsigned int j = 0; j < nb_it; j++)
    {
        int nbParticle = 1000;
        float* pos = new float[nbParticle * 3];

        initPosition(pos, nbParticle, bbox);

        pConstructData container;

        // Could use the Block constructor  BlockField(Block<3>& block, mapConstruct map = mapConstruct())
        // But would cost a copy
        BlockField domainData(true);    // Initialized the memory
        Block<3>* domainBlock = domainData.getBlock();
        vector<unsigned int> extendsBlock = {0,0,0,10,10,10};
        vector<float> bboxBlock = {0.0,0.0,0.0,10.0,10.0,10.0};
        domainBlock->setGridspace(1.0);
        domainBlock->setGlobalExtends(extendsBlock);
        domainBlock->setLocalExtends(extendsBlock);
        domainBlock->setOwnExtends(extendsBlock);
        domainBlock->setGlobalBBox(bboxBlock);
        domainBlock->setLocalBBox(bboxBlock);
        domainBlock->setOwnBBox(bboxBlock);
        domainBlock->ghostSize_ = 0;

        ArrayFieldf array(pos, 3*nbParticle, 3, false);
        SimpleFieldi data(nbParticle);

        container->appendData(string("nbParticules"), data,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_MINUS_NBITEM, DECAF_MERGE_ADD_VALUE);
        container->appendData(string("pos"), array,
                             DECAF_POS, DECAF_PRIVATE,
                             DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        container->appendData("domain_block", domainData,
                             DECAF_NOFLAG, DECAF_SHARED,
                             DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
        cout<<"End of construction of the data model."<<endl;
        cout<<"Number of particles in the data model : "<<container->getNbItems()<<endl;


        stringstream filename;
        filename<<j<<"_"<<filebasename<<rank<<"_before.ply";

        posToFile(array.getArray(), array.getNbItems(), filename.str(),r,g,b);

        redist->process(container, DECAF_REDIST_SOURCE);
        redist->flush();

        delete [] pos;
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
        redist = new RedistBlockMPI(0, nb_client, nb_client, nb_server, MPI_COMM_WORLD, DECAF_REDIST_P2P);
    else
        redist = new RedistBlockCCI(0, nb_client, nb_client, nb_server,
                                                global_id, MPI_COMM_WORLD,
                                                string("testcci.txt"),
                                                DECAF_REDIST_P2P);

    // TODO: test the case with 0
    srand(1 + global_id);
    unsigned int r,g,b;
    r = rand() % 255;
    g = rand() % 255;
    b = rand() % 255;


    for (unsigned int i = 0; i < nb_it; i++)
    {
        pConstructData result;
        redist->process(result, decaf::DECAF_REDIST_DEST);
        redist->flush();

        cout<<"==========================="<<endl;
        cout<<"Final Merged map has "<<result->getNbItems()<<" items."<<endl;
        cout<<"Final Merged map has "<<result->getMap()->size()<<" fields."<<endl;
        result->printKeys();
        //printMap(*result);
        cout<<"==========================="<<endl;

        stringstream filename;
        filename<<i<<"_"<<filebasename<<rank<<".ply";

        ArrayFieldf pos = result->getFieldData<ArrayFieldf>("pos");
        posToFile(pos.getArray(), pos.getNbItems(), filename.str(),r,g,b);
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

    uint32_t caps	= 0;
    int ret = cci_init(CCI_ABI_VERSION, 0, &caps);
    if (ret)
    {
        fprintf(stderr, "cci_init() failed with %s\n",
            cci_strerror(NULL, (cci_status)ret));
        exit(EXIT_FAILURE);
    }

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
        fprintf(stderr, "Using the CCI redistribution component.\n");

    if(is_client)
        run_client(nb_client, nb_server, nb_it, use_mpi);
    if(is_server)
        run_server(nb_client, nb_server, nb_it, use_mpi);

    cci_finalize();

    MPI_Finalize();

}
