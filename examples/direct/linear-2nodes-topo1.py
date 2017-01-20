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
topoProd = topo.subTopology("prod", 4, 0)
topoDflow = topo.subTopology("dflow", 2, 4)
topoCon = topo.subTopology("con", 2, 6)

# Creating the graph
w = nx.DiGraph()
w.add_node("prod", topology=topoProd, func='prod', cmdline='linear_2nodes')
w.add_node("con", topology=topoCon, func='con', cmdline='linear_2nodes')
w.add_edge("prod", "con", topology=topoDflow, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='linear_2nodes')


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "linear2", mod_path)
