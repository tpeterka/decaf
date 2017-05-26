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
#include <bredala/data_model/constructtype.h>
#include <bredala/data_model/simpleconstructdata.hpp>


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
int arrayAlloc();
void multiToFlat(float *mu1, float *mu2, float *mu3);
float *A3D(unsigned columns, unsigned rows, unsigned floors);

int getIndex(int i, int j, int k, int x, int y, int z) {
    return ((i*y*z)+(j*z))+k;
}

struct detector {
    char myname[32];
    float merror;
    float reduct;
    float margin;
    float reopen;
    int nlearn;
    int nbasic;
    int repair;
    int tmstep;
};

int detectorInit (struct detector *d, const char *name, int nlearn, int repair, int margin) {
    d->merror = 0.0;
    sprintf(d->myname, "%s", name);
    d->nbasic = nlearn/5;
    d->nlearn = nlearn;
    d->reduct = 1.0;
    d->margin = (float) margin;
    d->reopen = (float) margin+5;
    d->repair = repair;
    d->tmstep = 0;
    return 0;
}


float predict(float *mu1, int x, int y, int z, int i, int j, int k) {
    float p1 = mu1[getIndex(i-1,j,k,x,y,z)] + (mu1[getIndex(i+1,j,k,x,y,z)] - mu1[getIndex(i-1,j,k,x,y,z)])/2;
    float p2 = mu1[getIndex(i,j-1,k,x,y,z)] + (mu1[getIndex(i,j+1,k,x,y,z)] - mu1[getIndex(i,j-1,k,x,y,z)])/2;
    float p3 = mu1[getIndex(i,j,k-1,x,y,z)] + (mu1[getIndex(i,j,k+1,x,y,z)] - mu1[getIndex(i,j,k-1,x,y,z)])/2;
    return (p1+p2+p3)/3;
}

float updateError(float value, float predi, struct detector *d) {
    float err = fabs(value - predi);

    if ((d->tmstep > d->nbasic) && (d->tmstep < d->nlearn) && (err > d->merror))
    {
        d->margin = d->margin + 5;
        d->reopen = d->reopen + 5;
    }
    if (err > (d->merror*((100.0-d->margin)/100.0))) {
        d->merror = err*(((100.0+d->reopen)/100.0));
        //printf("RANK %d : Error updated to %f \n", rank, d->merror);
    }
    return err;
}

int pointCheck(float *mu1, int index, float predi, struct detector *d)
{
    int det = 0;
    if (d->tmstep < d->nlearn)
    {
        updateError(mu1[index], predi, d);
    } else {
        float er = fabs(mu1[index] - predi);
        if (er > d->merror)
        {
            det = 1;
            printf("%s: Corruption detected : %f > %f \n", d->myname, er, d->merror);
            if (d->repair)
            {
                printf("%s: Corruption corrected : %f => %f \n", d->myname, mu1[index], predi);
                mu1[index] = predi;
            }
        } else {
            if (er > (d->merror*((100-d->margin)/100)))
            {
                updateError(mu1[index], predi, d);
            }
        }
    }
    return det;
}

int checkSDC3(float *mu1, int x, int y, int z, struct detector *d) {
    int i, j, k, det = 0;
    for (i = 1; i < x-1; i++)
    {
        for (j = 1; j < y-1; j++)
        {
            for (k = 1; k < z-1; k++)
            {
                int index = getIndex(i,j,k,x,y,z);
                float predi = predict(mu1, x, y, z, i, j, k);
                det = det + pointCheck(mu1, index, predi, d);
            }
        }
    }
    if (d->tmstep > d->nlearn && det == 0) {
        d->merror = d->merror*((100.0-d->reduct)/100.0);
        //printf("RANK %d : Error reduced to %f \n", rank, d->merror);
    }
    d->tmstep++;
 
    return det;
}


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

float *mu1, *mu2, *mu3, *uptr;
struct detector d1, d2;

// node and link callback functions
extern "C"
{
    // producer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void prod(void* args,                       // arguments to the callback
              int t_current,                    // current time step
              int t_interval,                   // consumer time interval
              int t_nsteps,                     // total number of time steps
              vector<Dataflow*>* in_dataflows,  // all inbound dataflows
              vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        //fprintf(stderr, "prod\n");

        float* pd = (float*)args;               // producer data
        *pd = (float)t_current;                 // just assign something, current time step eg.
        MPI_Comm globalComm = (*out_dataflows)[0]->prod_comm_handle();

        std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();

        if (t_current == 0)
        {
            Comm *myComm = (*out_dataflows)[0]->prod_comm();
            int numprocs = myComm->size();
            int myid = myComm->rank();

            Initialize(myid);
            int mem = (arrayAlloc()*sizeof(float))/(1024*1024);
            initialCond(myid, numprocs);
            if (myid == 0) printf("%d MB of memory per process.\n", mem);

            mu1 = A3D(LE+2, HI+2, DE+2);
            mu2 = A3D(LE+2, HI+2, DE+2);
            mu3 = A3D(LE+2, HI+2, DE+2);
        }

        if (t_current < t_nsteps)         // Main loop
        { // Time stepping

            Comm *myComm = (*out_dataflows)[0]->prod_comm();
            int numprocs = myComm->size();
            int myid = myComm->rank();
            int nnext = (myid + 1)%numprocs;
            int previous = (myid + numprocs - 1)%numprocs;

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
            for (size_t i = 0; i < out_dataflows->size(); i++)
            {
                multiToFlat(mu1, mu2, mu3);
                std::shared_ptr<VectorConstructData<float> > array = make_shared< VectorConstructData<float> >(mu1, (LE+1)*(HI+1)*(DE+1), 1);
                container->appendData(std::string("speed"), array,
                         DECAF_NOFLAG, DECAF_PRIVATE,
                         DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
                //fprintf(stderr, "+++ producing time step %d \n", t_current);
                (*out_dataflows)[i]->put(container, DECAF_PROD);
                (*out_dataflows)[i]->flush();
                //std::cout<<"Prod Put done"<<std::endl;
            }
        }

        if (t_current == t_nsteps-1)        // Last iteration
        {
            free(mu1);
            free(mu2);
            free(mu3);
        }
    }

    // consumer
    // check your modulo arithmetic to ensure you get exactly con_nsteps times
    void con(void* args,                       // arguments to the callback
             int t_current,                    // current time step
             int t_interval,                   // consumer time interval
             int t_nsteps,                     // total number of time steps
             vector<Dataflow*>* in_dataflows,  // all inbound dataflows
             vector<Dataflow*>* out_dataflows) // all outbound dataflows
    {
        if (t_current == 0)
        {
            Comm *myComm = (*in_dataflows)[0]->con_comm();
            int myid = myComm->rank();
            Initialize(myid);
            uptr = A3D(LE+2, HI+2, DE+2);
            detectorInit(&d1, "space", 50, 0, 10);
            //detectorInit(&d2, "times", 50, 0, 20);
        }
        if (!((t_current + 1) % t_interval))
        {

            Comm *myComm = (*in_dataflows)[0]->con_comm();
            int myid = myComm->rank();

            //std::cout<<"Con"<<std::endl;
            //int* cd    = (int*)in_dataflows[0]->get(); // we know dataflow.size() = 1 in this example
            std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
            (*in_dataflows)[0]->get(container, DECAF_CON);
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
            //checkSDC3(uptr, LE+2, HI+2, DE+2, t_current, 0);
            checkSDC3(uptr, LE+2, HI+2, DE+2, &d1);
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
               Dataflow* dataflow)           // dataflow
    {
        //Getting the data from the producer
        std::shared_ptr<ConstructData> container = std::make_shared<ConstructData>();
        dataflow->get(container, DECAF_DFLOW);
        //std::cout<<"dflow get done"<<std::endl;

        //Forwarding the data to the consumers
        dataflow->put(container, DECAF_DFLOW);
        dataflow->flush();
        //std::cout<<"dflow put done"<<std::endl;
    }
} // extern "C"

void run(Workflow& workflow,                 // workflow
         int prod_nsteps,                    // number of producer time steps
         int con_nsteps)                     // number of consumer time steps
{
    // callback args
    int *pd, *cd;
    pd = new int[1];
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].func == "prod")
            workflow.nodes[i].args = pd;
        if (workflow.nodes[i].func == "con")
            workflow.nodes[i].args = cd;
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
    path.append(string("/examples/cfd/libmod_cfd.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                  // producer
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "prod";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                  // consumer
    node.in_links.clear();
    node.in_links.push_back(0);
    node.start_proc = 2;
    node.nprocs = 2;
    node.func = "con";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow link
    WorkflowLink link;
    link.prod = 0;                           // dataflow
    link.con = 1;
    link.start_proc = 1;
    link.nprocs = 2;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // run decaf
    run(workflow, prod_nsteps, con_nsteps);

    return 0;
}
