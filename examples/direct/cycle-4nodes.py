# a small 4-node example that contains cycles

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_cycle_4nodes.so'

# define workflow graph
# 4-node graph with 2 cycles
#
#      __  node_b (1 proc)
#     /   /
#    node_a (4 procs)
#         \
#          node_c (1 proc) - node_d (1 proc)
#
#
#  entire workflow takes 11 procs (1 dataflow proc between each producer consumer pair)
#  all graph nodes and links are on disjoint processes in this example

w = nx.DiGraph()

w.add_node("node_b", start_proc=5, nprocs=1, func='node_b', cmdline='./cycle_4nodes')
w.add_node("node_d", start_proc=9, nprocs=1, func='node_d', cmdline='./cycle_4nodes')
w.add_node("node_c", start_proc=7, nprocs=1, func='node_c', cmdline='./cycle_4nodes')
w.add_node("node_a", start_proc=0, nprocs=4, func='node_a', cmdline='./cycle_4nodes', tokens = 1)

w.add_edge("node_c", "node_d", start_proc=8,  nprocs=1, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='./cycle_4nodes')
w.add_edge("node_a", "node_b", start_proc=4,  nprocs=1, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='./cycle_4nodes')
w.add_edge("node_a", "node_c", start_proc=6,  nprocs=1, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='./cycle_4nodes')
w.add_edge("node_b", "node_a", start_proc=10, nprocs=1, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='./cycle_4nodes')

# --- convert the nx graph into a workflow data structure and generate the JSON ---
wf.processGraph(w, "cycle")
