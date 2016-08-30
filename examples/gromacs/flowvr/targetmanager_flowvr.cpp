//---------------------------------------------------------------------------
//
// 2-node producer-consumer coupling example
//
// prod (4 procs) - con (2 procs)
//
// entire workflow takes 8 procs (2 dataflow procs between prod and con)
// this file contains the consumer (2 procs)
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <decaf/data_model/simplefield.hpp>
#include <decaf/data_model/arrayfield.hpp>
#include <decaf/data_model/blockfield.hpp>
#include <decaf/data_model/boost_macros.h>

#include <decaf/data_model/morton.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <set>

#include "Damaris.h"

#include <decaf/workflow.hpp>

#include "flowvr/module.h"

using namespace decaf;
using namespace std;
using namespace boost;

flowvr::ModuleAPI* Module;
flowvr::BufferPool* pOutPool;
flowvr::OutputPort *outPositions;
flowvr::OutputPort *outTargets;
flowvr::OutputPort *outSelection;
flowvr::StampInfo StampType("TypePositions",flowvr::TypeInt::create()); 	//0 = No filtering, 1 or 2 = filtered data. We swap between 1 and 2 when a new filter arrive

enum targetType {
    ABS = 0,
    REL
};

#define MAX_SIZE_REQUEST 2048
typedef struct
{
    int type;                   //Target absolute in space or relative to a reference point
    float target[3];            //Coordonates of the absolute target
    char targetRequest[2048];   //Request of the
}Target;

float length(float* vec){
    return sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

void normalize(float* vec){
    float l = length(vec);
    vec[0] = vec[0] / l;
    vec[1] = vec[1] / l;
    vec[2] = vec[2] / l;
}

float distance(float* p1, float* p2)
{
    float vec[3];
    vec[0] = p2[0] - p1[0];
    vec[1] = p2[1] - p1[1];
    vec[2] = p2[2] - p1[2];
    return length(vec);
}

std::set<int> filterIds;
std::vector<int> arrayIds; //HARD CODED for SimpleWater example

std::vector<Target> targets;

float distanceValidTarget = 0.5;
float maxTotalForces = 3.f;
std::string model;

int nbHomeAtoms = -1;

void loadTargets()
{
    if(model.compare(std::string("SimplePeptideWater")) == 0)
    {
        Target target;
        target.target[0] = 4.0;
        target.target[1] = 6.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        target.target[0] = 23.0;
        target.target[1] = 6.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        target.target[0] = 23.0;
        target.target[1] = 38.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        target.target[0] = 4.0;
        target.target[1] = 38.0;
        target.target[2] = 20.0;
        targets.push_back(target);

        filterIds = {  109, 110, 111, 112, 113, 114,
                       115, 116, 117, 118, 119, 120
                    }; //HARD CODED for SimpleWater example

        arrayIds = {  109, 110, 111, 112, 113, 114,
                      115, 116, 117, 118, 119, 120
                   }; //HARD CODED for SimpleWater example
    }
    else if(model.compare(std::string("fepa")) == 0)
    {
        Target target;
        target.target[0] = 50.649998;
        target.target[1] = 40.020000;
        target.target[2] = 74.940002;
        targets.push_back(target);

        target.target[0] = 57.994247;
        target.target[1] = 42.744064;
        target.target[2] = 75.205559;
        targets.push_back(target);

        target.target[0] = 58.028599;
        target.target[1] = 39.480324;
        target.target[2] = 62.716755;
        targets.push_back(target);

        target.target[0] = 58.175446;
        target.target[1] = 36.721069;
        target.target[2] = 59.135941;
        targets.push_back(target);

        target.target[0] = 60.568310;
        target.target[1] = 35.987762;
        target.target[2] = 56.373985;
        targets.push_back(target);

        target.target[0] = 57.443069;
        target.target[1] = 41.200779;
        target.target[2] = 52.448627;
        targets.push_back(target);

        target.target[0] = 60.272179;
        target.target[1] = 41.596397;
        target.target[2] = 41.934307;
        targets.push_back(target);

        target.target[0] = 58.013557;
        target.target[1] = 49.347263;
        target.target[2] = 14.191130;
        targets.push_back(target);

        //Ids for ENT and FE residues
        for(int i = 69901; i <= 69952; i++)
        {
            filterIds.insert(i);
            arrayIds.push_back(i);
        }
    }
}

std::vector<float> targetsArray;


// consumer
void target(Decaf* decaf)
{
    vector< pConstructData > in_data;
    fprintf(stderr, "Launching target\n");
    fflush(stderr);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int iteration = 0;

    loadTargets();

    unsigned int currentTarget = 0;

    while (decaf->get(in_data))
    {
        ArrayFieldf posField = in_data[0]->getFieldData<ArrayFieldf>("pos");
        if(!posField)
        {
            fprintf(stderr, "ERROR : the field \'pos\' is not in the data model\n");
            continue;
        }

        float* pos = posField.getArray();

        ArrayFieldu idsField = in_data[0]->getFieldData<ArrayFieldu>("ids");
        if(!idsField)
        {
            fprintf(stderr, "ERROR : the field \'ids\' is not in the data model\n");
            continue;
        }

        unsigned int* ids = idsField.getArray();

        unsigned int nbAtoms = posField->getNbItems();

        float avg[3];
        bzero(avg, 3 * sizeof(float));

        // Filtering the data to select only the steered complex
        for(unsigned int i = 0; i < nbAtoms; i++)
        {
            if(filterIds.count(ids[i]) != 0)
            {
                avg[0] += pos[3*i];
                avg[1] += pos[3*i+1];
                avg[2] += pos[3*i+2];
            }
        }

        // Computing the average position of the steered system
        avg[0] = avg[0] / (float)filterIds.size();
        avg[1] = avg[1] / (float)filterIds.size();
        avg[2] = avg[2] / (float)filterIds.size();

        fprintf(stderr, "Average position : %f %f %f\n", avg[0], avg[1], avg[2]);

        // Checking the position between the system and the target
        float* targetPos = targets[currentTarget].target;
        float dist = distance(avg, targetPos);
        if(dist < distanceValidTarget)
        {
            //Changing the active target
            currentTarget++;
            fprintf(stderr, "Changing target\n");
            if(currentTarget < targets.size())
                targetPos = targets[currentTarget].target;
            else
            {
                fprintf(stderr, "Steering terminated. Closing the app\n");
                break;
            }
        }
        fprintf(stderr, "[%i/%u] Target position : %f %f %f\n", currentTarget, targets.size(), targetPos[0], targetPos[1], targetPos[2]);
        fprintf(stderr, "[%i/%u] Distance to target : %f\n", currentTarget, targets.size(), dist);
        // Computing the force direction
        float force[3];
        force[0] = targetPos[0] - avg[0];
        force[1] = targetPos[1] - avg[1];
        force[2] = targetPos[2] - avg[2];

        normalize(force);

        // Computing the force intensity
        float maxForcesPerAtom = maxTotalForces / (float)filterIds.size();
        //force[0] = force[0] * maxForcesPerAtom;
        //force[1] = force[1] * maxForcesPerAtom;
        //force[2] = force[2] * maxForcesPerAtom;
        bzero(force, 3 * sizeof(float));
        fprintf(stderr, "Force emitted : %f %f %f\n", force[0], force[1], force[2]);

        // Generating the data model
        pConstructData container;
        ArrayFieldf field(force, 3, 3, false);
        container->appendData("force", field, DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_FIRST_VALUE);
        if(iteration == 0)
        {
            ArrayFieldi fieldIds(&arrayIds[0], arrayIds.size(), 1, false);
            container->appendData("ids", fieldIds, DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);
        }

        decaf->put(container);

        //Clearing the first time as the data model will change at the next iteration
        if(iteration == 0)
            decaf->clearBuffers(DECAF_NODE);

        // FlowVR forwarding
        // We recreate the order of data
        if(!Module->wait())
        {
            fprintf(stderr, "Closed by FlowVR\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // We don't know the total number of atoms
        // without the copies from the redistribution
        if(nbHomeAtoms < 0)
        {
            for(unsigned int i = 0; i < nbAtoms; i++)
                nbHomeAtoms = max(nbHomeAtoms, (int)ids[i]);
            nbHomeAtoms++; // The IDs start at 0
        }

        flowvr::MessageWrite msgPos;
        msgPos.data = pOutPool->alloc(Module->getAllocator(), nbHomeAtoms * 3 * sizeof(float));
        float* atompos = (float*)(msgPos.data.writeAccess());

        //Move all the position outside of the cone of vision
        for(unsigned int i = 0; i < nbHomeAtoms * 3; i++)
            atompos[i] = 100000.0f;

        //Putting all the non water atom positions
        for(unsigned int i = 0; i < nbAtoms; i++)
        {
            //HARD CODED FOR FEPA
            if(ids[i] <= 17432 || ids[i] >= 69901)
            {
                atompos[3*ids[i]] = pos[3*i];
                atompos[3*ids[i]+1] = pos[3*i+1];
                atompos[3*ids[i]+2] = pos[3*i+2];
            }
        }
        msgPos.stamps.write(StampType, 0);

        Module->put(outPositions, msgPos);

        //Putting the target positions
        if(targetsArray.empty())
        {
            for(unsigned int i = 0; i < targets.size(); i++)
            {
                targetsArray.push_back(targets[i].target[0]);
                targetsArray.push_back(targets[i].target[1]);
                targetsArray.push_back(targets[i].target[2]);
            }
        }

        flowvr::MessageWrite msgTargets;
        msgTargets.data = pOutPool->alloc(Module->getAllocator(), targetsArray.size() * sizeof(float));
        memcpy(msgTargets.data.writeAccess(), &targetsArray[0], msgTargets.data.getSize());

        Module->put(outTargets, msgTargets);

        //Putting the current position of the steered group
        flowvr::MessageWrite msgPosSelection;
        msgPosSelection.data = pOutPool->alloc(Module->getAllocator(), 3 * sizeof(float));
        memcpy(msgPosSelection.data.writeAccess(), avg, msgPosSelection.data.getSize());

        Module->put(outSelection, msgPosSelection);

        iteration++;
    }

    Module->close();

    if(pOutPool) delete pOutPool;

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    fprintf(stderr, "Target terminating\n");
    decaf->terminate();
}

// every user application needs to implement the following run function with this signature
// run(Workflow&) in the global namespace
void run(Workflow& workflow)                             // workflow
{
    MPI_Init(NULL, NULL);

    char processorName[MPI_MAX_PROCESSOR_NAME];
    int size_world, rank, nameLen;

    MPI_Comm_size(MPI_COMM_WORLD, &size_world);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processorName,&nameLen);

    srand(time(NULL) + rank * size_world + nameLen);

    fprintf(stderr, "target rank %i\n", rank);

    // create decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);

    //FlowVR initialization
    outPositions = new flowvr::OutputPort("outPos");
    outPositions->stamps->add(&StampType);
    outTargets = new flowvr::OutputPort("outTargets");
    outSelection = new flowvr::OutputPort("outSelection");

    std::vector<flowvr::Port*> ports;
    ports.push_back(outPositions);
    ports.push_back(outTargets);
    ports.push_back(outSelection);

    //Module = flowvr::initModule(ports, traces, string("decaf"));
    Module = flowvr::initModule(ports);

    if(Module == NULL){
        fprintf(stderr, "Erreur : Module treatment_flowvr non initialise.");
        return ;
    }

    pOutPool = new flowvr::BufferPool();

    // start the task
    target(decaf);

    // cleanup
    delete decaf;
    fprintf(stderr,"Decaf deleted. Waiting on finalize\n");
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    fprintf(stderr, "Hello treatment\n");

    // define the workflow
    Workflow workflow;
    Workflow::make_wflow_from_json(workflow, "wflow_gromacs.json");

    if(argc != 4)
    {
        fprintf(stderr, "Usage : targetmanager_flowvr profile maxforce disttotarget\n");
        return 0;
    }

    model = string(argv[1]);
    maxTotalForces = atof(argv[2]);
    distanceValidTarget = atof(argv[3]);

    // run decaf
    run(workflow);

    return 0;
}
