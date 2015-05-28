# a small 2-node example, just a producer and consumer

import networkx as nx
import os

# --- set your options here ---

# path to .so for driver
driver_path = os.environ['DECAF_PREFIX'] + '/examples/lammps/python/libpy_lammps.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/lammps/libmod_lammps.so'

# define workflow graph
# 2-node workflow
#
#    lammps (4 procs) - print (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()

# example of 4 nodes and 3 edges (single source)
# this is the example diagrammed above, and for which driver.pyx is made
w.add_node("lammps", start_proc=0, nprocs=4, prod_func='lammps', con_func=''       ,
           path=mod_path)
w.add_node("print",  start_proc=6, nprocs=2, prod_func= ''     , con_func='print'  ,
           path=mod_path)
w.add_edge("lammps", "print", start_proc=4, nprocs=2           , dflow_func='dflow',
           path=mod_path)

# total number of time steps
prod_nsteps  = 1
con_nsteps   = 1

# lammps input file
infile = "/Users/tpeterka/software/decaf/examples/lammps/in.melt"

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver', driver_path)
driver.pyrun(w, prod_nsteps, con_nsteps, infile)
