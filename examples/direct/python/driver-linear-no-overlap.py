# a small 3-node linear example, node1 -> node2 -> node3

import networkx as nx
import os

# --- set your options here ---

# path to .so for driver
driver_path = os.environ['DECAF_PREFIX'] + '/examples/direct/python/libpy_linear_3_nodes.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/libmod_linear_3_nodes.so'

# define workflow graph
# 3-node linear workflow
#
#    node1 (4 procs) - node2 (3 procs) - node3 (2 procs)
#
#  entire workflow takes 12 procs (2 dataflow procs between node1 and node2, and
#  1 dataflow node between node2 and node3)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()
w.add_node("node1", start_proc=0,  nprocs=4, func='node1', type='prod', path=mod_path)
w.add_node("node2", start_proc=6,  nprocs=3, func='node2', type='both', path=mod_path)
w.add_node("node3", start_proc=10, nprocs=2, func='node3', type='con' , path=mod_path)
w.add_edge("node1", "node2", start_proc=4, nprocs=2, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')
w.add_edge("node2", "node3", start_proc=9, nprocs=1, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')

# total number of time steps
prod_nsteps  = 2
con_nsteps   = 2

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver_redist', driver_path)
driver.pyrun(w, prod_nsteps, con_nsteps)
