
# --- set your options here ---

# path to .so module
path = '/Users/tpeterka/software/decaf/install/examples/lammps/python/libpy_lammps.so'

# communicator sizes and time steps
class DecafSizes:
    prod_size    = 4      # size of producer communicator
    dflow_size   = 2      # size of dataflow communicator
    con_size     = 2      # size of consumer communicator
    prod_start   = 0      # starting rank of producer communicator in the world
    dflow_start  = 4      # starting rank of dataflow communicator in the world
    con_start    = 6      # starting rank of consumer communicator in the world
    con_nsteps   = 0      # number of consumer (decaf) time steps

# total number of time steps; this is the user's variable, not part of decaf
prod_nsteps  = 0

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
w.add_node("lammps", start_proc=0, nprocs=4, prod_func='lammps'     , con_func=''          )
w.add_node("print",  start_proc=6, nprocs=2, prod_func= ''          , con_func='print'     )
w.add_edge("lammps", "print", start_proc=4, nprocs=2, dflow_func='dflow'                   )

# total number of time steps
prod_nsteps  = 1
con_nsteps   = 1

# lammps input file
infile = "/Users/tpeterka/software/decaf/examples/lammps/in.melt"

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver', path)
driver.pyrun(w, prod_nsteps, con_nsteps, infile)
