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
        string prod_func
        string con_func
        void* prod_args
        void* con_args
        string path
    struct WorkflowLink:
        int prod
        int con
        int start_proc
        int nprocs
        string dflow_func
        void* dflow_args
        string path
    struct Workflow:
        vector[WorkflowNode] nodes
        vector[WorkflowLink] links

cdef extern from "../examples/direct/direct.cpp":
    void run(Workflow& workflow, int prod_nsteps, int con_nsteps)

def pyrun(workflow, prod_nsteps, con_nsteps, pathfunc):
    cdef WorkflowNode wnode
    cdef WorkflowLink wlink
    cdef Workflow wflow

    # parse workflow and fill C++ struct

    # iterate over nodes
    i = 0
    for node in workflow.nodes_iter(data=True):
        wnode.start_proc = node[1]['start_proc']
        wnode.nprocs     = node[1]['nprocs']
        wnode.prod_func  = node[1]['prod_func']
        wnode.con_func   = node[1]['con_func']
        wnode.path       = node[1]['path']
        node[1]['index'] = i                         # add identifier to each node
        i += 1

        # add the node to the vector of workflow nodes
        wflow.nodes.push_back(wnode)

    # iterate over edges
    i = 0
    for edge in workflow.edges_iter(data=True):
        wlink.prod       = workflow.node[edge[0]]['index']
        wlink.con        = workflow.node[edge[1]]['index']
        wlink.start_proc = edge[2]['start_proc']
        wlink.nprocs     = edge[2]['nprocs']
        wlink.dflow_func = edge[2]['dflow_func']
#        wlink.path       = edge[2]['path']
        wlink.path       = pathfunc

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
    run(wflow, prod_nsteps, con_nsteps)
