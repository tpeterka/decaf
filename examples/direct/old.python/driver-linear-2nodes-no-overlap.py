# a small 2-node example, just a producer and consumer

import networkx as nx
import os

# --- set your options here ---

# path to .so for driver
driver_path = os.environ['DECAF_PREFIX'] + '/examples/direct/python/py_linear_2nodes.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_2nodes.so'

# define workflow graph
# 2-node workflow
#
#    lammps (4 procs) - print (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()
w.add_node("prod", start_proc=0, nprocs=4, func='prod', path=mod_path)
w.add_node("con",  start_proc=6, nprocs=2, func='con' , path=mod_path)
w.add_edge("prod", "con", start_proc=4, nprocs=2, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')

# sources
sources = [0]

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver_linear_2nodes', driver_path)
driver.pyrun(w, sources)
