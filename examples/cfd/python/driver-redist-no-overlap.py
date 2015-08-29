# a small 2-node example, just a producer and consumer

import networkx as nx
import os

# --- set your options here ---

# path to .so for driver
driver_path = os.environ['DECAF_PREFIX'] + '/examples/cfd/python/libpy_cfd_redist.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/cfd/libmod_cfd_redist.so'

# define workflow graph
# 2-node workflow
#
#    lammps (4 procs) - print (2 procs)
#
#  entire workflow takes 9 procs

w = nx.DiGraph()
w.add_node("prod", start_proc=0, nprocs=4, func='prod', path=mod_path)
w.add_node("con",  start_proc=5, nprocs=4, func='con' , path=mod_path)
w.add_edge("prod", "con", start_proc=4, nprocs=1, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')

# total number of time steps
prod_nsteps  = 10
con_nsteps   = 10

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver_redist', driver_path)
driver.pyrun(w, prod_nsteps, con_nsteps)
