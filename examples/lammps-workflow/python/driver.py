import networkx as nx

# --- set your options here ---

# path to .so module
path = '/Users/tpeterka/software/decaf/install/examples/lammps-workflow/python/libpy_lammps_workflow.so'

# define workflow graph
# 4-node workflow with 3 decaf instances (one per link)
#
#          print1 (1 proc)
#        /
#    lammps (4 procs)
#        \
#          print2 (1 proc) - print3 (1 proc)
#
#  entire workflow takes 10 procs (1 dataflow proc between each producer consumer pair)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()
w.add_node("lammps", start_proc=0, nprocs=4)
w.add_node("print1", start_proc=5, nprocs=1)
w.add_node("print2", start_proc=7, nprocs=1)
w.add_node("print3", start_proc=9, nprocs=1)
w.add_edge("lammps", "print1", start_proc=4, nprocs=1)
w.add_edge("lammps", "print2", start_proc=6, nprocs=1)
w.add_edge("print2", "print3", start_proc=8, nprocs=1)

# debug
print w.nodes(data=True)
print w.edges(data=True)

# total number of time steps
prod_nsteps  = 1
con_nsteps   = 1

# lammps input file
infile = "/Users/tpeterka/software/decaf/examples/lammps-workflow/in.melt"

# --- do not edit below this point --

# call driver

import imp
driver = imp.load_dynamic('driver', path)
driver.pyrun(w, prod_nsteps, con_nsteps, infile)
