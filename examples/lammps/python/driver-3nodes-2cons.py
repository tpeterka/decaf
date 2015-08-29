# a 4-node workflow example

import networkx as nx
import os

# --- set your options here ---

# path to .so for driver
driver_path = os.environ['DECAF_PREFIX'] + '/examples/lammps/python/libpy_lammps.so'

# path to .so module for callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/lammps/libmod_lammps.so'

# define workflow graph
# 4-node workflow
#
#          print1 (1 proc)
#        /
#    lammps (4 procs)
#        \
#          print2 (1 proc)
#
#  entire workflow takes 8 procs (1 dataflow proc between each producer consumer pair)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()

# example of 3 nodes and 2 edges (single source)
# this is the example diagrammed above, and for which driver.pyx is made
w.add_node("lammps", start_proc=0, nprocs=4, func='lammps', path=mod_path)
w.add_node("print1", start_proc=5, nprocs=1, func='print' , path=mod_path)
w.add_node("print2", start_proc=7, nprocs=1, func='print2', path=mod_path)
w.add_edge("lammps", "print1", start_proc=4, nprocs=1, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')
w.add_edge("lammps", "print2", start_proc=6, nprocs=1, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count')

# total number of time steps
prod_nsteps  = 1
con_nsteps   = 1

# lammps input file
infile = os.environ['DECAF_PREFIX'] + "/examples/lammps/in.melt"

# --- do not edit below this point --

# call driver

import imp
driver = imp.load_dynamic('driver', driver_path)
driver.pyrun(w, prod_nsteps, con_nsteps, infile)
