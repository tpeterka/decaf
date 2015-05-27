# a small 2-node example, just a producer and consumer

import networkx as nx
import os

# --- set your options here ---

# path to .so module
#path = '/Users/tpeterka/software/decaf/install/examples/direct/python/libpy_direct.so'
path= os.environ["DECAF_PREFIX"]+"/examples/direct/python/libpy_direct.so"
pathfunc = os.environ["DECAF_PREFIX"]+"/examples/direct/libmod_direct.so"

# define workflow graph
# 2-node workflow
#
#    lammps (4 procs) - print (2 procs)
#
#  entire workflow takes 4 procs because of the overlap

w = nx.DiGraph()
w.add_node("prod", start_proc=0, nprocs=4, prod_func='prod'     , con_func=''        )
w.add_node("con",  start_proc=2, nprocs=2, prod_func= ''        , con_func='con'     )
w.add_edge("prod", "con", start_proc=1, nprocs=2, dflow_func='dflow'                 )

# total number of time steps
prod_nsteps  = 1
con_nsteps   = 1

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver', path)
driver.pyrun(w, prod_nsteps, con_nsteps, pathfunc)
