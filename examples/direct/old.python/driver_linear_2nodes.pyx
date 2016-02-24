import networkx as nx
from libcpp.vector cimport vector
from libcpp.string cimport string

cdef extern from "decaf/decaf.hpp":
    pass

cdef extern from "decaf/types.hpp":
    struct WorkflowNode:
        vector[int] out_links
        vector[int] in_links
        int start_proc
        int nprocs
        string func
        void* args
        string path
    struct WorkflowLink:
        int prod
        int con
        int start_proc
        int nprocs
        string func
        void* args
        string path
        string prod_dflow_redist
        string dflow_con_redist
    struct Workflow:
        vector[WorkflowNode] nodes
        vector[WorkflowLink] links

cdef extern from "../examples/direct/linear_2nodes.cpp":
    void run(Workflow& workflow, vector[int]& sources)

def pyrun(workflow, sources):
    cdef WorkflowNode wnode
    cdef WorkflowLink wlink
    cdef Workflow wflow

    # parse workflow and fill C++ struct

    # iterate over nodes
    i = 0
    for node in workflow.nodes_iter(data=True):
        wnode.start_proc = node[1]['start_proc']
        wnode.nprocs     = node[1]['nprocs']
        wnode.func       = node[1]['func']
        wnode.path       = node[1]['path']
        node[1]['index'] = i                         # add identifier to each node
        i += 1

        # add the node to the vector of workflow nodes
        wflow.nodes.push_back(wnode)

    # iterate over edges
    i = 0
    for edge in workflow.edges_iter(data=True):
        wlink.prod              = workflow.node[edge[0]]['index']
        wlink.con               = workflow.node[edge[1]]['index']
        wlink.start_proc        = edge[2]['start_proc']
        wlink.nprocs            = edge[2]['nprocs']
        wlink.func              = edge[2]['func']
        wlink.path              = edge[2]['path']
        wlink.prod_dflow_redist = edge[2]['prod_dflow_redist']
        wlink.dflow_con_redist  = edge[2]['dflow_con_redist']

        # add edge to corresponding nodes
        wflow.nodes[wlink.prod].out_links.push_back(i)
        wflow.nodes[wlink.con].in_links.push_back(i)
        i += 1

        # add the link to the vector of workflow links
        wflow.links.push_back(wlink)

    # debug
#     print 'wnodes:', wflow.nodes
#     print 'wlinks:', wflow.links

    # run the workflow
    run(wflow, sources)
