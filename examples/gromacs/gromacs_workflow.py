# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/gromacs/libmod_dflow_gromacs.so'

# define workflow graph
# 2-node workflow
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()
w.add_node("gmx", start_proc=0, nprocs=4, func='gmx')
w.add_node("treatment",  start_proc=6, nprocs=2, func='treatment')
w.add_node("target",  start_proc=9, nprocs=1, func='target')
w.add_edge("gmx", "treatment", start_proc=4, nprocs=2, func='dflow_morton', path=mod_path,
           prod_dflow_redist='proc', dflow_con_redist='block')
w.add_edge("treatment", "target", start_proc=8, nprocs=1, func='dflow_simple', path=mod_path,
           prod_dflow_redist='proc', dflow_con_redist='proc')
w.add_edge("target", "gmx", start_proc=10, nprocs=1, func='dflow_simple', path=mod_path,
           prod_dflow_redist='proc', dflow_con_redist='proc')

# --- convert the nx graph into a workflow data structure and run the workflow ---

wf.workflowToJson(w, mod_path, "wflow_gromacs.json")
