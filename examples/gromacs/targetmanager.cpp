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
#include <decaf/data_model/array3dconstructdata.hpp>
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
#include <climits>

// For path finding
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <vector>
#include <utility>
#include <queue>
#include <tuple>
#include <algorithm>

#include "Damaris.h"

#include <decaf/workflow.hpp>

#ifdef FLOWVR
#include "flowvr/module.h"
flowvr::ModuleAPI* Module;
flowvr::BufferPool* pOutPool;
flowvr::OutputPort *outPositions;
flowvr::OutputPort *outTargets;
flowvr::OutputPort *outSelection;
flowvr::StampInfo StampType("TypePositions",flowvr::TypeInt::create()); 	//0 = No filtering, 1 or 2 = filtered data. We swap between 1 and 2 when a new filter arrive
#endif

using std::unordered_map;
using std::unordered_set;
using std::array;
using std::vector;
using std::queue;
using std::priority_queue;
using std::pair;
using std::tuple;
using std::tie;
using std::string;

using namespace decaf;
using namespace std;

// Helpers for SquareGrid::Location

// When using std::unordered_map<T>, we need to have std::hash<T> or
// provide a custom hash function in the constructor to unordered_map.
// Since I'm using std::unordered_map<tuple<int,int>> I'm defining the
// hash function here. It would be nice if C++ automatically provided
// the hash function for tuple and pair, like Python does. It would
// also be nice if C++ provided something like boost::hash_combine. In
// any case, here's a simple hash function that combines x and y:
namespace std {
  template <>
  struct hash<tuple<int,int,int> > {
    inline size_t operator()(const std::tuple<int,int,int>& location) const {
      int x, y, z;
      tie (x, y, z) = location;
      return x * 1812433253 + y;
    }
  };
}

struct SquareGrid {
  typedef std::tuple<int,int,int> Location;
  static std::array<Location, 26> DIRS;

  int width, height, depth;
  unordered_set<Location> walls;

  SquareGrid(int width_, int height_, int depth_)
     : width(width_), height(height_), depth(depth_) {}

  inline bool in_bounds(Location id) const {
    int x, y, z;
    tie (x, y, z) = id;
    return 0 <= x && x < width && 0 <= y && y < height && 0 <= z && z < depth;
  }

  inline bool passable(Location id) const {
    return !walls.count(id);
  }

  std::vector<Location> neighbors(Location id) const {
    int x, y, z, dx, dy, dz;
    std::tie (x, y, z) = id;
    std::vector<Location> results;

    for (auto dir : DIRS) {
      std::tie (dx, dy, dz) = dir;
      Location next(x + dx, y + dy, z + dz);
      if (in_bounds(next) && passable(next)) {
        results.push_back(next);
      }
    }

    //if ((x + y) % 2 == 0) {
      // aesthetic improvement on square grids
    //  std::reverse(results.begin(), results.end());
    //}

    return results;
  }
};

struct GridWithWeights: SquareGrid {
  //nordered_set<Location> forests;
  boost::multi_array<float, 3>*  grid_;
  GridWithWeights(int w, int h, int d, boost::multi_array<float, 3>*  grid): SquareGrid(w, h, d) {grid_ = grid;}
  double cost(Location from_node, Location to_node) const {
    //return forests.count(to_node) ? 5 : 1;
      return 1;
  }
};

template<typename T, typename priority_t>
struct PriorityQueue {
  typedef pair<priority_t, T> PQElement;
  priority_queue<PQElement, vector<PQElement>,
                 std::greater<PQElement>> elements;

  inline bool empty() const { return elements.empty(); }

  inline void put(T item, priority_t priority) {
    elements.emplace(priority, item);
  }

  inline T get() {
    T best_item = elements.top().second;
    elements.pop();
    return best_item;
  }
};

template<typename Location>
vector<Location> reconstruct_path(
   Location start,
   Location goal,
   unordered_map<Location, Location>& came_from
) {
  vector<Location> path;
  Location current = goal;
  path.push_back(current);
  while (current != start) {
    current = came_from[current];
    path.push_back(current);
  }
  path.push_back(start); // optional
  std::reverse(path.begin(), path.end());
  return path;
}

inline double heuristic(SquareGrid::Location a, SquareGrid::Location b) {
  int x1, y1, z1, x2, y2, z2;
  std::tie (x1, y1, z1) = a;
  std::tie (x2, y2, z2) = b;
  return abs(x1 - x2) + abs(y1 - y2) + abs(z1 - z2);
}

template<typename Graph>
void a_star_search
  (const Graph& graph,
   typename Graph::Location start,
   typename Graph::Location goal,
   unordered_map<typename Graph::Location, typename Graph::Location>& came_from,
   unordered_map<typename Graph::Location, double>& cost_so_far)
{
  typedef typename Graph::Location Location;
  PriorityQueue<Location, double> frontier;
  frontier.put(start, 0);

  came_from[start] = start;
  cost_so_far[start] = 0;

  while (!frontier.empty()) {
    auto current = frontier.get();

    if (current == goal) {
      break;
    }

    for (auto next : graph.neighbors(current)) {
      double new_cost = cost_so_far[current] + graph.cost(current, next);
      if (!cost_so_far.count(next) || new_cost < cost_so_far[next]) {
        cost_so_far[next] = new_cost;
        double priority = new_cost + heuristic(next, goal);
        frontier.put(next, priority);
        came_from[next] = current;
      }
    }
  }
}

//array<SquareGrid::Location, 4> SquareGrid::DIRS {Location{1, 0}, Location{0, -1}, Location{-1, 0}, Location{0, 1}};
std::array<SquareGrid::Location, 26> SquareGrid::DIRS {Location{-1, -1, -1}, Location{-1, -1, 0}, Location{-1, -1, 1}, Location{-1, 0, -1}, Location{-1, 0, 0}, Location{-1, 0, 1}, Location{-1, 1, -1}, Location{-1, 1, 0}, Location{-1, 1, 1}, Location{0, -1, -1}, Location{0, -1, 0}, Location{0, -1, 1}, Location{0, 0, -1}, Location{0, 0, 1}, Location{0, 1, -1}, Location{0, 1, 0}, Location{0, 1, 1}, Location{1, -1, -1}, Location{1, -1, 0}, Location{1, -1, 1}, Location{1, 0, -1}, Location{1, 0, 0}, Location{1, 0, 1}, Location{1, 1, -1}, Location{1, 1, 0}, Location{1, 1, 1}};

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
    std::vector<float> targetsArray;

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

        std::shared_ptr<Array3DConstructData<float> > grid =
                in_data[0]->getTypedData<Array3DConstructData<float> >("grid");

        unsigned int* ids = idsField.getArray();

        unsigned int nbAtoms = posField->getNbItems();

        float avg[3];
        bzero(avg, 3 * sizeof(float));
        int count = 0;

        // Filtering the data to select only the steered complex
        for(unsigned int i = 0; i < nbAtoms; i++)
        {
            if(filterIds.count(ids[i]) != 0)
            {
                avg[0] = avg[0] + pos[3*i];
                avg[1] = avg[1] + pos[3*i+1];
                avg[2] = avg[2] + pos[3*i+2];
                count++;
            }
        }

        // Computing the average position of the steered system
        avg[0] = avg[0] / (float)count;
        avg[1] = avg[1] / (float)count;
        avg[2] = avg[2] / (float)count;

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
        fprintf(stderr, "[%i/%u] Target position: %f %f %f\n", currentTarget, targets.size(), targetPos[0], targetPos[1], targetPos[2]);
        fprintf(stderr, "[%i/%u] Distance to target: %f\n", currentTarget, targets.size(), dist);

        float force[3];

        if(grid)
        {
            fprintf(stderr, "Grid available, we are using path finding.\n");
            // Computing the force direction
            force[0] = targetPos[0] - avg[0];
            force[1] = targetPos[1] - avg[1];
            force[2] = targetPos[2] - avg[2];

            normalize(force);

            // Computing the force intensity
            float maxForcesPerAtom = maxTotalForces / (float)filterIds.size();
            force[0] = force[0] * maxForcesPerAtom;
            force[1] = force[1] * maxForcesPerAtom;
            force[2] = force[2] * maxForcesPerAtom;

            //Block<3> block = grid->getBlock();

            // Getting the grid info
            BlockField blockField  = in_data[0]->getFieldData<BlockField>("domain_block");
            if(!blockField)
            {
                fprintf(stderr, "ERROR: the field \'domain_block\' is not in the data model\n");
                continue;
            }
            Block<3>* domainBlock = blockField.getBlock();

            // We use the domain because we gather the full grid on 1 node

            // Using the local coords because the 3D array is a local
            // representation, not own representation
            unsigned int avgIndex[3];
            if(!domainBlock->getLocalPositionIndex(avg, avgIndex))
            {
                fprintf(stderr," ERROR: Unable to get the correct index of the tracked position\n");
                continue;
            }
            unsigned int targetIndex[3];
            if(!domainBlock->getLocalPositionIndex(targetPos,targetIndex))
            {
                fprintf(stderr," ERROR: Unable to get the correct index of the target position\n");
                continue;
            }

            fprintf(stderr,"Track cell: [%u %u %u]\n", avgIndex[0], avgIndex[1], avgIndex[2]);
            fprintf(stderr,"Target cell: [%u %u %u]\n", targetIndex[0], targetIndex[1], targetIndex[2]);

            // Now we can do the path finding
            //std::vector<NodePath> path;
            //bool foundPath = findPath(grid->getArray(),
            //              avgIndex,
            //              targetIndex,
            //              path)
            boost::multi_array<float, 3> *densityGrid = grid->getArray();
            GridWithWeights gridForest(
                        densityGrid->shape()[0],
                        densityGrid->shape()[1],
                        densityGrid->shape()[2],
                        densityGrid);
            SquareGrid::Location start{avgIndex[0], avgIndex[1], avgIndex[2]};
            SquareGrid::Location goal{targetIndex[0], targetIndex[1], targetIndex[2]};
            unordered_map<SquareGrid::Location, SquareGrid::Location> came_from;
            unordered_map<SquareGrid::Location, double> cost_so_far;
            a_star_search(gridForest, start, goal, came_from, cost_so_far);
            vector<SquareGrid::Location> path = reconstruct_path(start, goal, came_from);
            fprintf(stderr, "Length of the path : %zu\n", path.size());
            for(unsigned int i = 0; i < path.size(); i++)
            {
                int x, y, z;
                std::tie (x, y, z) = path[i];
                fprintf(stderr, "Location %u : [%i %i %i]\n",i,x,y,z);

                // Converting the index into spatial coords
                float targetCoord[3];
                unsigned int targetIndex[3];
                targetIndex[0] = x;
                targetIndex[1] = y;
                targetIndex[2] = z;
                domainBlock->getGlobalPositionValue(targetIndex, targetCoord);

                fprintf(stderr, "Intermediate target %u: [%f %f %f]\n", i, targetCoord[0], targetCoord[1], targetCoord[2]);
            }
            // We need at last 2 points (origin destination).
            // If only one point, then the track is already in the target cell
            bool foundPath = path.size() > 1;
            foundPath=false;
            if(foundPath && domainBlock->hasLocalBBox_)
            {
                fprintf(stderr, "We found a proper path. Adjusting the course.\n");
                int x,y,z;
                float targetCoord[3];
                unsigned int targetIndex[3];

                // Looking for the closest target > maxDistanceTarget
                for(unsigned int i = 0; i < path.size(); i++)
                {
                    std::tie (x, y, z) = path[1];   // Next sub target in line

                    // Converting the index into spatial coords
                    targetIndex[0] = x;
                    targetIndex[1] = y;
                    targetIndex[2] = z;
                    domainBlock->getGlobalPositionValue(targetIndex, targetCoord);

                    float dist = distance(avg, targetCoord);

                    // No risk to go backward because the path starts at the tracking pos
                    if(dist > distanceValidTarget)
                    {
                        fprintf(stderr," The closest non reached target is %u\n",i);
                        break;
                    }
                }

                fprintf(stderr, "Intermediate target to reach: [%f %f %f]\n", targetCoord[0], targetCoord[1], targetCoord[2]);
                force[0] = targetCoord[0] - avg[0];
                force[1] = targetCoord[1] - avg[1];
                force[2] = targetCoord[2] - avg[2];
            }
            else
            {
                fprintf(stderr, "Was unable to find a proper path. Using direct mode instead\n");
                // Computing the force direction
                force[0] = targetPos[0] - avg[0];
                force[1] = targetPos[1] - avg[1];
                force[2] = targetPos[2] - avg[2];
            }

        }
        else
        {
            fprintf(stderr, "Computing direct force\n");
            // Computing the force direction
            force[0] = targetPos[0] - avg[0];
            force[1] = targetPos[1] - avg[1];
            force[2] = targetPos[2] - avg[2];
        }

        normalize(force);

        // Computing the force intensity
        float maxForcesPerAtom = maxTotalForces / (float)filterIds.size();
        force[0] = force[0] * maxForcesPerAtom;
        force[1] = force[1] * maxForcesPerAtom;
        force[2] = force[2] * maxForcesPerAtom;


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

#ifdef FLOWVR
        // FlowVR forwarding
        // We recreate the order of data
        if(!Module->wait())
        {
            fprintf(stderr, "Closed by FlowVR\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        else
            fprintf(stderr,"FlowVR wait passed\n");

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
            //17432 for the channel
            //69901 for the iron complex
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
#endif
        iteration++;
    }

#ifdef FLOWVR
    Module->close();

    if(pOutPool) delete pOutPool;
#endif

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

    if(decaf->con_comm_size() != 1)
    {
        fprintf(stderr, "ERROR: The TargetManager is supposed to run on only 1 MPI rank. Abording.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
#ifdef FLOWVR
    std::cout<<"Initialization of Flowvr."<<std::endl;
    //FlowVR initialization
    outPositions = new flowvr::OutputPort("outPos");
    outPositions->stamps->add(&StampType);
    outTargets = new flowvr::OutputPort("outTargets");
    outSelection = new flowvr::OutputPort("outSelection");

    std::vector<flowvr::Port*> ports;
    ports.push_back(outPositions);
    ports.push_back(outTargets);
    ports.push_back(outSelection);

    Module = flowvr::initModule(ports);

    if(Module == NULL){
        fprintf(stderr, "Erreur : Module treatment_flowvr non initialise.");
        return ;
    }

    pOutPool = new flowvr::BufferPool();
#endif
    std::cout<<"Context initialized."<<std::endl;

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
    //make_wflow(workflow);
    Workflow::make_wflow_from_json(workflow, "wflow_gromacs.json");

    if(argc != 4)
    {
        fprintf(stderr, "Usage : targetmanager profile maxforce disttotarget\n");
        return 0;
    }

    model = string(argv[1]);
    maxTotalForces = atof(argv[2]);
    distanceValidTarget = atof(argv[3]);

    // run decaf
    run(workflow);

    return 0;
}
