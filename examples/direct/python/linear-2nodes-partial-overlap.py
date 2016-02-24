# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to python run .so
run_path = os.environ['DECAF_PREFIX'] + '/examples/direct/pybind11/py_linear_2nodes.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_2nodes.so'

# define workflow graph
# 2-node workflow
#
#    lammps (4 procs) - print (2 procs)
#
#  entire workflow takes 4 procs because of the overlap

w = nx.DiGraph()
w.add_node("prod", start_proc=0, nprocs=4, func='prod', path=mod_path)
w.add_node("con",  start_proc=2, nprocs=2, func='con' , path=mod_path)
w.add_edge("prod", "con", start_proc=1, nprocs=2, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')

# sources
source_nodes = ['prod']

# --- convert the nx graph into a workflow data structure and run the workflow ---

wf.workflow(w,                               # nx workflow graph
            source_nodes,                    # source nodes in the workflow
            'py_linear_2nodes',              # run module name
            run_path)                        # run path
