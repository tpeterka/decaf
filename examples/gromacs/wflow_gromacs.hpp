//---------------------------------------------------------------------------
//
// workflow definition
//
// prod (2 procs) - con (1 procs)
//
// entire workflow takes 4 procs (1 dataflow procs between prod and con)
//
// Matthieu Dreher
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// mdreher@anl.gov
//
//--------------------------------------------------------------------------

#include <decaf/decaf.hpp>
#include <cstdlib>

using namespace decaf;
using namespace std;

// generates workflow for debugging purposes
// this is hard-coding the no overlap case
void make_wflow(Workflow& workflow)

{
    char * prefix = getenv("DECAF_PREFIX");
    if (prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. "
                "Please export DECAF_PREFIX to point to the root of your decaf "
                "install directory.\n");
        exit(1);
    }

    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/gromacs/libmod_dflow_gromacs.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                        // producer
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 2;
    node.func = "gmx";
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // consumer
    node.in_links.clear();
    node.in_links.push_back(0);
    node.start_proc = 3;
    node.nprocs = 1;
    node.func = "treatment";
    workflow.nodes.push_back(node);

    // fill workflow link
    WorkflowLink link;
    link.prod = 0;                                // dataflow
    link.con = 1;
    link.start_proc = 2;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);
}
