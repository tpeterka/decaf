//---------------------------------------------------------------------------
//
// workflow definition
//
// prod (8 procs) -> dflow (4 procs) -> tess (2 procs) -> dflow (2 procs) -> dense (2 procs)
//
// entire workflow takes 18 procs
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
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

    string path1 = string(prefix , strlen(prefix));
    path1.append(string("/examples/hacc/mod_hacc_dflow.so"));
    string path2 = string(prefix , strlen(prefix));
    path2.append(string("/examples/hacc/mod_tess_dflow.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.out_links.clear();                        // prod
    node.in_links.clear();
    node.out_links.push_back(0);
    node.start_proc = 0;
    node.nprocs = 8;
    node.func = "prod";
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // tess
    node.in_links.clear();
    node.in_links.push_back(0);
    node.out_links.push_back(1);
    node.start_proc = 12;
    node.nprocs = 2;
    node.func = "tessellate";
    workflow.nodes.push_back(node);

    node.out_links.clear();                        // dense
    node.in_links.clear();
    node.in_links.push_back(1);
    node.start_proc = 16;
    node.nprocs = 2;
    node.func = "density_estimate";
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 0;                                // prod->tess
    link.con = 1;
    link.start_proc = 8;
    link.nprocs = 4;
    link.func = "dflow1";
    link.path = path1;
    link.prod_dflow_redist = "proc";
    link.dflow_con_redist = "block";
    workflow.links.push_back(link);

    link.prod = 1;                                // tess->dense
    link.con = 2;
    link.start_proc = 14;
    link.nprocs = 2;
    link.func = "dflow2";
    link.path = path2;
    link.prod_dflow_redist = "count";        // TODO: change to proc when receiving actual data
    link.dflow_con_redist = "count";         // TODO: change to proc when sending actual data
    workflow.links.push_back(link);

    // a temporary test of just prod and tess
    // WorkflowNode node;
    // node.out_links.clear();                        // prod
    // node.in_links.clear();
    // node.out_links.push_back(0);
    // node.start_proc = 0;
    // node.nprocs = 8;
    // node.func = "hacc";
    // workflow.nodes.push_back(node);

    // node.out_links.clear();                        // tess
    // node.in_links.clear();
    // node.in_links.push_back(0);
    // node.start_proc = 9;
    // node.nprocs = 1;
    // node.func = "tessellate";
    // workflow.nodes.push_back(node);

    // // fill workflow links
    // WorkflowLink link;
    // link.prod = 0;                                // prod->tess
    // link.con = 1;
    // link.start_proc = 8;
    // link.nprocs = 1;
    // link.func = "dflow1";
    // link.path = path;
    // link.prod_dflow_redist = "proc";
    // link.dflow_con_redist = "block";
    // workflow.links.push_back(link);
}
