# a small 3-node linear example, node1 -> node2 -> node3

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to .so for driver
run_path = os.environ['DECAF_PREFIX'] + '/examples/direct/python/py_linear_3nodes.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_3nodes.so'

# define workflow graph
# 3-node linear workflow
#
#    node0 (4 procs) -> node1 (3 procs) -> node2 (2 procs)
#
#  entire workflow takes 4 procs because of total overlap

w = nx.DiGraph()
w.add_node('node0', start_proc=0,  nprocs=4, func='node0', path=mod_path)
w.add_node('node1', start_proc=0,  nprocs=2, func='node1', path=mod_path)
w.add_node('node2', start_proc=0,  nprocs=1, func='node2', path=mod_path)
w.add_edge('node0', 'node1', start_proc=0, nprocs=3, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')
w.add_edge('node1', 'node2', start_proc=0, nprocs=2, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')

# --- convert the nx graph into a workflow data structure and run the workflow ---

wf.workflow(w,                               # nx workflow graph
            run_path)                        # run path
