//---------------------------------------------------------------------------
//
// example of direct coupling
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------
#include <decaf/decaf.hpp>
#include <decaf/data_model/constructtype.h>
#include <decaf/data_model/simpleconstructdata.hpp>


#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>

using namespace decaf;
using namespace std;

void BounCondInGhostCells(void);
void Reconstruction(int next, int previous, MPI_Comm globalComm);
void BounCondOnInterfaces( void );
void Fluxes(int next, int previous, MPI_Comm globalComm);
void Evolution(int Stage);
int Initialize(int myid);
int initialCond(int myid, int numprocs);
int arrayAlloc(float *mu1, float *mu2, float *mu3, float *mu4, float *mu5);
int checkSDC3(float *mu1, int x, int y, int z, int step, int repair); 
int multiToFlat(float *mu1, float *mu2, float *mu3, float *mu4, float *mu5);
float *A3D(unsigned columns, unsigned rows, unsigned floors);
int freeMem(float *mu1, float *mu2, float *mu3, float *mu4, float *mu5);

// user-defined pipeliner code
void pipeliner(Dataflow* decaf)
{
}

// user-defined resilience code
void checker(Dataflow* decaf)
{
}

int LE = 80;
int HI = 90;
int DE = 90;   
        
float *mu1, *mu2, *mu3, *mu4, *mu5;
float* uptr;


// node and link callback functions
extern "C"
{
    // producer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void prod(void* args,                   // arguments to the callback
              int t_current,                // current time step
              int t_interval,               // consumer time interval
              int t_nsteps,                 // total number of time steps
              vector<Dataflow*>* dataflows, // all dataflows (for producer or consumer)
              int this_dataflow = -1)       // index of one dataflow in list of all

    {
        //fprintf(stderr, "prod\n");

        float* pd = (float*)args;               // producer data
        *pd = (float)t_current;                    // just assign something, current time step for example
        MPI_Comm globalComm = (*dataflows)[0]->prod_comm_handle(); 

        std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
        
        if (t_current == 0)
        {
            Comm *myComm = (*dataflows)[0]->prod_comm();
            int numprocs = myComm->size();
            int myid = myComm->rank();

            Initialize(myid);
            int mem = (arrayAlloc(mu1, mu2, mu3, mu4, mu5)*sizeof(float))/(1024*1024);
            initialCond(myid, numprocs);
            if (myid == 0) printf("%d MB of memory per process.\n", mem);

            mu1 = A3D(LE+2, HI+2, DE+2);
            mu2 = A3D(LE+2, HI+2, DE+2);
            mu3 = A3D(LE+2, HI+2, DE+2);
            mu4 = A3D(LE+2, HI+2, DE+2);
            mu5 = A3D(LE+2, HI+2, DE+2);
        }

        if (t_current < t_nsteps)         // Main loop
        { // Time stepping

            Comm *myComm = (*dataflows)[0]->prod_comm();
            int numprocs = myComm->size();
            int myid = myComm->rank();
            int nnext = (myid + 1)%numprocs;
            int previous = (myid + numprocs - 1)%numprocs;

                //multiToFlat(mu1, mu2, mu3, mu4, mu5);
                //checkSDC3(mu1, LEN+2, HIG+2, DEP+2, step, 0);
                //checkSDC3(mu2, LEN+2, HIG+2, DEP+2, step, 0);
                //checkSDC3(mu3, LEN+2, HIG+2, DEP+2, step, 0);
                //checkSDC3(mu4, LEN+2, HIG+2, DEP+2, step, 0);
                //if (step % OSTEP == 0) Output(myid, argv[2], step, mu1); // Take frame
                //flatToMulti(mu1, mu2, mu3, mu4, mu5);
            //}
            BounCondInGhostCells();
            Reconstruction(nnext, previous, globalComm);
            BounCondOnInterfaces();
            Fluxes(nnext, previous, globalComm);
            Evolution(1);

            BounCondInGhostCells();
            Reconstruction(nnext, previous, globalComm);
            BounCondOnInterfaces();
            Fluxes(nnext, previous, globalComm);
            Evolution(2);
            if (myid == 1) printf("Step %d ....\n", t_current);
        } // end simulation

        if (!((t_current + 1) % t_interval))
        {
            for (size_t i = 0; i < dataflows->size(); i++)
            {
                multiToFlat(mu1, mu2, mu3, mu4, mu5);
                std::shared_ptr<VectorConstructData<float> > array = make_shared< VectorConstructData<float> >(mu1, (LE+1)*(HI+1)*(DE+1), 1);
                container->appendData(std::string("speed"), array,
                         DECAF_NOFLAG, DECAF_PRIVATE,
                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
                //fprintf(stderr, "+++ producing time step %d \n", t_current);
                (*dataflows)[i]->put(container, DECAF_PROD);
                (*dataflows)[i]->flush();
                //std::cout<<"Prod Put done"<<std::endl;
            }
        }

        if (t_current == t_nsteps-1)        // Last iteration
        {
            freeMem(mu1, mu2, mu3, mu4, mu5);
        }
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void con(void* args,                     // arguments to the callback
            int t_current,                  // current time step
            int t_interval,                 // consumer time interval
            int t_nsteps,                   // total number of time steps
            vector<Dataflow*>* dataflows,   // all dataflows (for producer or consumer)
            int this_dataflow = -1)         // index of one dataflow in list of all
    {
        if (t_current == 0)
        {
            Comm *myComm = (*dataflows)[0]->con_comm();
            int myid = myComm->rank();
            Initialize(myid);
            uptr = A3D(LE+2, HI+2, DE+2);
        }
        if (!((t_current + 1) % t_interval))
        {

            Comm *myComm = (*dataflows)[0]->con_comm();
            int myid = myComm->rank();
      
            //std::cout<<"Con"<<std::endl;
            //int* cd    = (int*)dataflows[0]->get(); // we know dataflow.size() = 1 in this example
            std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
            (*dataflows)[0]->get(container, DECAF_CON);
            //std::cout<<"Get done"<<std::endl;
            std::shared_ptr<VectorConstructData<float> > sum =
                dynamic_pointer_cast<VectorConstructData<float> >(container->getData(std::string("speed")));

            int i, j, k;
            vector<float> svec = sum->getVector();
            std::vector<float>::iterator it = svec.begin();
            for (i = 0; i <= LE+1; i++)
            {
                for (j = 0; j <= HI+1; j++)
                {
                    for (k = 0; k <= DE+1; k++)
                    {
                        uptr[(i*(HI+1)*(DE+1))+(j*(DE+1))+k] = (float) *it;
                        it++;
                    }
                }
            }

            //Output(myid, t_current, uptr);
            checkSDC3(uptr, LE+2, HI+2, DE+2, t_current, 0);
            // For this example, the policy of the redistribute component is add
            fprintf(stderr, "--- consuming time step %d, sum = %f\n", t_current, uptr[100]);
        }

        if (t_current == t_nsteps-1)        // Last iteration
        {
            free(uptr);
        }

    }

    // dataflow just needs to flush on every time step
    void dflow(void* args,                   // arguments to the callback
               int t_current,                // current time step
               int t_interval,               // consumer time interval
               int t_nsteps,                 // total number of time steps
               vector<Dataflow*>* dataflows, // all dataflows
               int this_dataflow = -1)       // index of one dataflow in list of all
    {
        //fprintf(stderr, "dflow\n");
        //for (size_t i = 0; i < dataflows.size(); i++)
        //  dataflows[i]->flush();               // need to clean up after each time step
        for (size_t i = 0; i < dataflows->size(); i++)
        {
            //Getting the data from the producer
            std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
            (*dataflows)[i]->get(container, DECAF_DFLOW);
            //std::cout<<"dflow get done"<<std::endl;

            //Forwarding the data to the consumers
            (*dataflows)[i]->put(container, DECAF_DFLOW);
            (*dataflows)[i]->flush();
            //std::cout<<"dflow put done"<<std::endl;
        }
    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int prod_nsteps,                    // number of producer time steps
         int con_nsteps)                     // number of consumer time steps
{
    // debug: print the workflow
    // WorkflowNode* n;
    // WorkflowLink* l;
    // n = &(workflow.nodes[0]);
    // fprintf(stderr, "node0: out %d start_proc %d nprocs %d prod_func %s con_func %s path %s\n",
    //         n->out_links[0], n->start_proc, n->nprocs, n->prod_func.c_str(), n->con_func.c_str(),
    //         n->path.c_str());
    // n = &(workflow.nodes[1]);
    // fprintf(stderr, "node1: in %d start_proc %d nprocs %d prod_func %s con_func %s path %s\n",
    //         n->in_links[0], n->start_proc, n->nprocs, n->prod_func.c_str(), n->con_func.c_str(),
    //         n->path.c_str());
    // l = &(workflow.links[0]);
    // fprintf(stderr, "link: prod %d con %d startproc %d nprocs %d dflow_func %s path %s\n",
    //         l->prod, l->con, l->start_proc, l->nprocs, l->dflow_func.c_str(), l->path.c_str());
    // fprintf(stderr, "prod_nsteps %d con_nsteps %d\n", prod_nsteps, con_nsteps);
 
    // debug: hard code the workflow
    // workflow.nodes.clear();
    // workflow.links.clear();
    // const char * prefix = getenv("DECAF_PREFIX");
    // string path = string(prefix , strlen(prefix));
    // path.append(string("/examples/direct/libmod_direct.so"));

    // // fill workflow nodes
    // WorkflowNode node;
    // node.out_links.clear();               // producer
    // node.in_links.clear();
    // node.out_links.push_back(0);
    // node.start_proc = 0;
    // node.nprocs = 4;
    // node.prod_func = "prod";
    // node.con_func = "";
    // node.path = path;
    // workflow.nodes.push_back(node);

    // node.out_links.clear();               // consumer
    // node.in_links.clear();
    // node.in_links.push_back(0);
    // // node.start_proc = 6;              // no overlap
    // node.start_proc = 2;                 // partial overlap
    // // node.start_proc = 0;              // total overlap
    // node.nprocs = 2;                     // no or partial overlap
    // // node.nprocs = 4;                  // total overlap
    // node.prod_func = "";
    // node.con_func = "con";
    // node.path = path;
    // workflow.nodes.push_back(node);

    // // fill workflow link
    // WorkflowLink link;
    // link.prod = 0;                       // dataflow
    // link.con = 1;
    // // link.start_proc = 4;              // no overlap
    // link.start_proc = 1;                 // partial overlap
    // // link.start_proc = 0;              // total overlap
    // link.nprocs = 2;                     // no or partial overlap
    // // link.nprocs = 4;                  // total overlap
    // link.dflow_func = "dflow";
    // link.path = path;
    // workflow.links.push_back(link);


    // callback args
    int *pd, *cd;
    pd = new int[1];
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].prod_func == "prod")
            workflow.nodes[i].prod_args = pd;
        if (workflow.nodes[i].prod_func == "con")
            workflow.nodes[i].prod_args = cd;
    }

    MPI_Init(NULL, NULL);

    // create and run decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow, prod_nsteps, con_nsteps);
    Data data(MPI_INT);
    decaf->run(&data, &pipeliner, &checker);

    // cleanup
    delete[] pd;
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// this is hard-coding the no overlap case
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    Workflow workflow;
    int prod_nsteps = 4;
    int con_nsteps = 4;
    char * prefix = getenv("DECAF_PREFIX");
    if(prefix == NULL)
    {
        std::cout<<"ERROR : environment variable DECAF_PREFIX not defined."
                <<" Please export DECAF_PREFIX to point to the root of you decaf install directory."<<std::endl;
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/libmod_direct.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                  // producer
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;
    node.prod_func = "prod";
    node.con_func = "";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                  // consumer
    node.in_links.clear();
    node.in_links.push_back(0);
    // node.start_proc = 6;                  // no overlap
    node.start_proc = 2;                     // partial overlap
    // node.start_proc = 0;                  // total overlap
    node.nprocs = 2;                         // no or partial overlap
    // node.nprocs = 4;                      // total overlap
    node.prod_func = "";
    node.con_func = "con";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow link
    WorkflowLink link;
    link.prod = 0;                           // dataflow
    link.con = 1;
    // link.start_proc = 4;                  // no overlap
    link.start_proc = 1;                     // partial overlap
    // link.start_proc = 0;                  // total overlap
    link.nprocs = 2;                         // no or partial overlap
    // link.nprocs = 4;                      // total overlap
    link.dflow_func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
