# a small 2-node example, just a producer and consumer

import networkx as nx
import os

# --- set your options here ---

# path to .so driver
driver_path = os.environ['DECAF_PREFIX'] + '/examples/direct/python/libpy_linear_2nodes.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/libmod_linear_2nodes.so'

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

# total number of time steps
prod_nsteps  = 3
con_nsteps   = 3

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver_linear_2nodes', driver_path)
driver.pyrun(w, prod_nsteps, con_nsteps)
