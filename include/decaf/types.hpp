//---------------------------------------------------------------------------
//
// decaf typedefs, enums
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_TYPES_HPP
#define DECAF_TYPES_HPP

#include <stdio.h>
#include <vector>
#include <queue>

using namespace std;

// data element for creating typemaps for datatypes
enum DispType
{
  DECAF_OFST,
  DECAF_ADDR,
  DECAF_NUM_DISP_TYPES,
};
struct DataMap {
  CommDatatype base_type; // existing datatype used to create this one
  DispType disp_type;     // diplacement is relative OFST or absolute ADDR
  int count;              // count of each element
  Address disp;           // starting displacement of each element in bytes
                          // OFSTs are from the start of the type and ADDRs are from 0x0
};

enum Decomposition
{
  DECAF_ROUND_ROBIN_DECOMP,
  DECAF_CONTIG_DECOMP,
  DECAF_ZCURVE_DECOMP,
  DECAF_NUM_DECOMPS,
};

// task types
typedef unsigned char TaskType;
#define DECAF_PROD      0x00
#define DECAF_DFLOW     0x01
#define DECAF_CON       0x02

// communicator types
typedef unsigned char CommType;
#define DECAF_OTHER_COMM      0x00
#define DECAF_PRODUCER_COMM   0x01
#define DECAF_DATAFLOW_COMM   0x02
#define DECAF_CONSUMER_COMM   0x04
#define DECAF_PROD_DFLOW_COMM 0x08
#define DECAF_DFLOW_CON_COMM  0x10

enum DecafError
{
  DECAF_OK,
  DECAF_COMM_SIZES_ERR,
  DECAF_NUM_ERRS,
};

struct DecafSizes
{
  int prod_size;         // size (number of processes) of producer communicator
  int dflow_size;        // size (number of processes) of dataflow communicator
  int con_size;          // size (number of processes) of consumer communicator
  int prod_start;        // starting world process rank of producer communicator
  int dflow_start;       // starting world process rank of dataflow communicator
  int con_start;         // starting world process rank of consumer communicator
  int con_nsteps;        // number of consumer timesteps
};

struct WorkflowNode      // a producer or consumer
{
  vector<int> out_links; // indices of outgoing links
  vector<int> in_links;  // indices of incoming links
  int start_proc;        // starting proc rank in world communicator for this producer or consumer
  int nprocs;            // number of processes for this producer or consumer
  char *prod_func;       // name of producer callback TODO: add args
  char *con_func;        // name of consumer callback TODO: add args
};

struct WorkflowLink      // a dataflow
{
  int prod;              // index in vector of all workflow nodes of producer
  int con;               // index in vector of all workflow nodes of consumer
  int start_proc;        // starting process rank in world communicator for the dataflow
  int nprocs;            // number of processes in the dataflow
  char*dflow_func;       // name of dataflow callback TODO: add args
};

struct Workflow          // an entire workflow
{
  vector<WorkflowNode> nodes; // all the workflow nodes
  vector<WorkflowLink> links; // all the workflow links
};

enum NodeColor           // colors of nodes in BFS search tree of the workflow
// TODO: switch to simple boolean visited
{
  WHITE,
  GRAY,
//   BLACK,
};

struct BFSNode           // one node in a breadth-first search tree of the workflow
{                        // TODO: switch to simple boolean visited
  NodeColor color;
//   int dist;
//   int parent;
};

void
all_err(int err_code)
{
  switch (err_code) {
  case DECAF_OK :
    break;
  case DECAF_COMM_SIZES_ERR :
    fprintf(stderr, "Decaf error: Group sizes of producer, consumer, and dataflow exceed total "
            "size of world communicator\n");
    break;
  default:
    break;
  }
}

// computes breadth-first search (BFS) tree of the graph [CLRS p. 532]
// TODO: remove distance and parent once I'm sure it is not needed
// TODO: remove BLACK color once I'm sure not needed
void
bfs(Workflow& workflow,
    vector<int>& sources,
    vector<int>& bfs_tree)
{
  vector<BFSNode> bfs_nodes;
  bfs_nodes.reserve(workflow.nodes.size());
  queue<int> q;

  // init all tree nodes
  for (size_t i = 0; i < workflow.nodes.size(); i++)
  {
    bfs_nodes[i].color  = WHITE;
//     bfs_nodes[i].dist   = -1;
//     bfs_nodes[i].parent = -1;
  }

  // init source tree nodes
  for (size_t i = 0; i < sources.size(); i++)
  {
    bfs_nodes[sources[i]].color = GRAY;
//     bfs_nodes[sources[i]].dist  = 0;
    q.push(sources[i]);
  }

  while (!q.empty())
  {
    int u = q.front();
    q.pop();
    bfs_tree.push_back(u);
    for (size_t i = 0; i < workflow.nodes[u].out_links.size(); i++)
    {
      int v = workflow.links[workflow.nodes[u].out_links[i]].con;
      if (bfs_nodes[v].color == WHITE)
      {
        bfs_nodes[v].color  = GRAY;
//         bfs_nodes[v].dist   = bfs_nodes[u].dist + 1;
//         bfs_nodes[v].parent = u;
        q.push(v);
      }
    }
//     bfs_nodes[u].color = BLACK;
  }
}

#define DECAF_DEBUG_MAX 256

void 
all_dbg( FILE* channel, char* msg)
{
#ifdef DECAF_DEBUG_ON 
  fprintf(channel, "[DECAF-DEBUG] %s", msg);
#endif
}

#endif
