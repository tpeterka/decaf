# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
import sys
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_2nodes.so'

# define workflow graph
# 2-node workflow
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# Creating the topology
topo = wf.topologyFromArgs(args)

# We want 50% of resources for prod, 25% for dflow, 25% for con
nprocs = topo.nProcs
if nprocs % 4 != 0:
    raise ValueError("The total number of MPI procs should be a multiple of 4 for this workflow.")
nprocsPerBlock = nprocs / 4

subtopos = topo.splitTopology(["prod","dflow","con"],[nprocsPerBlock*2,nprocsPerBlock,nprocsPerBlock])

# Creating the graph
w = nx.DiGraph()
w.add_node("prod", topology=subtopos[0], func='prod', cmdline='linear_2nodes')
w.add_node("con", topology=subtopos[2], func='con', cmdline='linear_2nodes')
w.add_edge("prod", "con", topology=subtopos[1], func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='linear_2nodes')


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "linear2", check_types=2)
