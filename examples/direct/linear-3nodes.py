# a small 3-node linear example, node1 -> node2 -> node3

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_3nodes.so'

# define workflow graph
# 3-node linear workflow
#
#    node0 (4 procs) -> node1 (3 procs) -> node2 (2 procs)
#
#  entire workflow takes 12 procs because of no overlap

# --- Graph definition ---
node0 = wf.Node("node0", start_proc=0, nprocs=4, func='node0', cmdline='./linear_3nodes')
outPort0 = node0.addOutputPort("out")

node1 = wf.Node("node1", start_proc=7, nprocs=2, func='node1', cmdline='./linear_3nodes')
inPort1 = node1.addInputPort("in")
outPort1 = node1.addOutputPort("out")

node2 = wf.Node("node2", start_proc=11, nprocs=1, func='node2', cmdline='./linear_3nodes')
inPort2 = node2.addInputPort("in")

link01 = wf.Edge(node0.getOutputPort("out"), node1.getInputPort("in"), start_proc=4, nprocs=3, func='dflow',
        path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./linear_3nodes')
link12 = wf.Edge(node1.getOutputPort("out"), node2.getInputPort("in"), start_proc=9, nprocs=2, func='dflow',
        path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./linear_3nodes')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("linear3")
