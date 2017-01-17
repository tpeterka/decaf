# a small 3-node example, synthetic point generation, tessellation, and density estimation

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/tess_dense/mod_pts_dflow.so'

# define workflow graph
# 3-node workflow
#
# points (8 procs) -> dflow1 (4 procs) -> tesselate (2 procs) -> dflow2 (2 procs) ->
# density_estimate (2 procs)
#
#  entire workflow takes 18 procs

w = nx.DiGraph()
w.add_node("prod",                           start_proc=0,  nprocs=4, func='prod')
w.add_node("tessellate",                     start_proc=8, nprocs=4, func='tessellate')
w.add_node("density_estimate",               start_proc=16, nprocs=4, func='density_estimate')
w.add_edge("prod", "tessellate",             start_proc=4,  nprocs=4, func='dflow1',
           path=mod_path, prod_dflow_redist='proc', dflow_con_redist='proc')
w.add_edge("tessellate", "density_estimate", start_proc=12,  nprocs=4, func='dflow2',
           path=mod_path, prod_dflow_redist='proc', dflow_con_redist='proc')

# --- convert the nx graph into a workflow data structure and run the workflow ---

wf.workflowToJson(w, mod_path, "tess_dense.json")
