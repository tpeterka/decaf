//---------------------------------------------------------------------------
//
// 4-node graph with 2 cycles
//
//      __  node_b (1 proc)
//     /   /
//    node_a (4 procs)
//         \
//          node_c (1 proc) - node_d (1 proc)
//             \ ______________ /
//
//  entire workflow takes 12 procs (1 dataflow proc between each producer consumer pair)
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
#include <decaf/data_model/arrayconstructdata.hpp>
#include <decaf/data_model/boost_macros.h>

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <utility>
#include <map>

using namespace decaf;
using namespace std;

// user-defined pipeliner code
void pipeliner(Dataflow* dataflow)
{
}

// user-defined resilience code
void checker(Dataflow* dataflow)
{
}

// node and link callback functions
extern "C"
{
    void node_a(void* args,                                   // arguments to the callback
                vector<Dataflow*>* out_dataflows,             // all outbound dataflows
                vector< shared_ptr<ConstructData> >* in_data) // input data in order of
                                                              // inbound dataflows
    {
        static bool first_time = true;
        static int sum = 0;
        shared_ptr<ConstructData> container;

        if (first_time)
        {
            // starting with sum = 0
            shared_ptr<SimpleConstructData<int> > data  =
                make_shared<SimpleConstructData<int> >(sum);
            fprintf(stderr, "node_a starting up value = %d\n", sum);

            // package it
            container = make_shared<ConstructData>();
            container->appendData(string("var"), data,
                                  DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
            first_time = false;
        }
        else
        {
            fprintf(stderr, "a: in_data size %ld\n", in_data->size());

            // get the values and add them
            for (size_t i = 0; i < in_data->size(); i++)
            {
                shared_ptr<BaseConstructData> ptr = (*in_data)[i]->getData(string("var"));
                if (ptr)
                {
                    shared_ptr<SimpleConstructData<int> > val =
                        dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                    sum += val->getData();
                    fprintf(stderr, "node_a: got value %d sum = %d\n", val->getData(), sum);
                }
            }

            // package the sum
            shared_ptr<SimpleConstructData<int> > data  =
                make_shared<SimpleConstructData<int> >(sum);
            container = make_shared<ConstructData>();
            container->appendData(string("var"), data,
                                  DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);
        }

        // send the sum
        for (size_t i = 0; i < out_dataflows->size(); i++)
            (*out_dataflows)[i]->put(container, DECAF_PROD);
    }

    void node_b(void* args,                                   // arguments to the callback
                vector<Dataflow*>* out_dataflows,             // all outbound dataflows
                vector< shared_ptr<ConstructData> >* in_data) // input data in order of
                                                              // inbound dataflows
    {
        int static sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data->size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = (*in_data)[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
                fprintf(stderr, "node_b: got value %d sum = %d\n", val->getData(), sum);
            }
        }

        // send the sum
        int sums[4] = {0, 0, 0, 0};
        shared_ptr<ArrayConstructData<int> > data  =
            make_shared<ArrayConstructData<int> >(sums, 4, 1, false);
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_DEFAULT, DECAF_MERGE_APPEND_VALUES);

        for (size_t i = 0; i < out_dataflows->size(); i++)
            (*out_dataflows)[i]->put(container, DECAF_PROD);
    }

    void node_c(void* args,                                   // arguments to the callback
                vector<Dataflow*>* out_dataflows,             // all outbound dataflows
                vector< shared_ptr<ConstructData> >* in_data) // input data in order of
                                                              // inbound dataflows
    {
        int static sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data->size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = (*in_data)[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
                fprintf(stderr, "node_c: got value %d sum = %d\n", val->getData(), sum);
                // debug
                (*in_data)[i]->printKeys();
            }
        }

        // send the sum
        shared_ptr<SimpleConstructData<int> > data  =
            make_shared<SimpleConstructData<int> >(sum);
        shared_ptr<ConstructData> container = make_shared<ConstructData>();
        container->appendData(string("var"), data,
                              DECAF_NOFLAG, DECAF_PRIVATE,
                              DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

        for (size_t i = 0; i < out_dataflows->size(); i++)
            (*out_dataflows)[i]->put(container, DECAF_PROD);
    }

    void node_d(void* args,                                   // arguments to the callback
                vector<Dataflow*>* out_dataflows,             // all outbound dataflows
                vector< shared_ptr<ConstructData> >* in_data) // input data in order of
                                                              // inbound dataflows
    {
        int static sum = 0;

        // get the values and add them
        for (size_t i = 0; i < in_data->size(); i++)
        {
            shared_ptr<BaseConstructData> ptr = (*in_data)[i]->getData(string("var"));
            if (ptr)
            {
                shared_ptr<SimpleConstructData<int> > val =
                    dynamic_pointer_cast<SimpleConstructData<int> >(ptr);
                sum += val->getData();
                fprintf(stderr, "node_d: got value %d sum = %d\n", val->getData(), sum);
            }
        }

        // send the sum if less than some amount (for example)
        if (sum < 100)
        {
            shared_ptr<SimpleConstructData<int> > data  =
                make_shared<SimpleConstructData<int> >(sum);
            shared_ptr<ConstructData> container = make_shared<ConstructData>();
            container->appendData(string("var"), data,
                                  DECAF_NOFLAG, DECAF_PRIVATE,
                                  DECAF_SPLIT_KEEP_VALUE, DECAF_MERGE_ADD_VALUE);

            for (size_t i = 0; i < out_dataflows->size(); i++)
                (*out_dataflows)[i]->put(container, DECAF_PROD);
        }
    }

    // dataflow just forwards everything that comes its way in this example
    void dflow(void* args,                          // arguments to the callback
               Dataflow* dataflow,                  // dataflow
               shared_ptr<ConstructData> in_data)  // input data
    {
        fprintf(stderr, "dflow:\n");

        // need to remove routing info from containter that put() inserted
        // TODO: fix dataflow->put() so that the user doesn't need to do this
        in_data->removeData("src_type");
        in_data->removeData("link_id");
        in_data->removeData("dest_id");

        dataflow->put(in_data, DECAF_DFLOW);
    }
} // extern "C"

void run(Workflow&    workflow,                     // workflow
         vector<int>& sources)                      // source workflow nodes
{
    // callback args
    for (size_t i = 0; i < workflow.nodes.size(); i++)
    {
        if (workflow.nodes[i].func == "node_a")
            ;                                       // TODO: set up args
        if (workflow.nodes[i].func == "node_b")
            ;                                       // TODO: set up args
        if (workflow.nodes[i].func == "node_c")
            ;                                       // TODO: set up args
        if (workflow.nodes[i].func == "node_d")
            ;                                       // TODO: set up args
    }

    MPI_Init(NULL, NULL);

    // create and run decaf
    Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);
    decaf->run(&pipeliner, &checker, sources);

    MPI_Barrier(MPI_COMM_WORLD);

    // cleanup
    delete decaf;
    MPI_Finalize();
}

// test driver for debugging purposes
// normal entry point is run(), called by python
int main(int argc,
         char** argv)
{
    Workflow workflow;
    char * prefix = getenv("DECAF_PREFIX");
    if (prefix == NULL)
    {
        fprintf(stderr, "ERROR: environment variable DECAF_PREFIX not defined. Please export "
                "DECAF_PREFIX to point to the root of your decaf install directory.\n");
        exit(1);
    }
    string path = string(prefix , strlen(prefix));
    path.append(string("/examples/direct/libmod_cycle_4nodes.so"));

    // fill workflow nodes
    WorkflowNode node;
    node.in_links.push_back(1);                     // node_b
    node.out_links.push_back(4);
    node.start_proc = 5;
    node.nprocs = 1;
    node.func = "node_b";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                         // node_d
    node.in_links.clear();
    node.in_links.push_back(0);
    node.out_links.push_back(3);
    node.start_proc = 9;
    node.nprocs = 1;
    node.func = "node_d";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                         // node_c
    node.in_links.clear();
    node.out_links.push_back(0);
    node.in_links.push_back(2);
    node.in_links.push_back(3);
    node.start_proc = 7;
    node.nprocs = 1;
    node.func = "node_c";
    node.path = path;
    workflow.nodes.push_back(node);

    node.out_links.clear();                         // node_a
    node.in_links.clear();
    node.out_links.push_back(1);
    node.out_links.push_back(2);
    node.in_links.push_back(4);
    node.start_proc = 0;
    node.nprocs = 4;
    node.func = "node_a";
    node.path = path;
    workflow.nodes.push_back(node);

    // fill workflow links
    WorkflowLink link;
    link.prod = 2;                                  // node_c -> node_d
    link.con = 1;
    link.start_proc = 8;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                                  // node_a -> node_b
    link.con = 0;
    link.start_proc = 4;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 3;                                  // node_a -> node_c
    link.con = 2;
    link.start_proc = 6;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 1;                                  // node_d -> node_c
    link.con = 2;
    link.start_proc = 10;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    link.prod = 0;                                  // node_b -> node_a
    link.con = 3;
    link.start_proc = 11;
    link.nprocs = 1;
    link.func = "dflow";
    link.path = path;
    link.prod_dflow_redist = "count";
    link.dflow_con_redist = "count";
    workflow.links.push_back(link);

    // identify sources
    vector<int> sources;
    sources.push_back(3);                           // node_a

    // run decaf
    run(workflow, sources);

    return 0;
}
