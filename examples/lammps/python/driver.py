
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

# lammps input file
infile = "/Users/tpeterka/software/decaf/examples/lammps/in.melt"

# --- do not edit below this point --

import imp
driver = imp.load_dynamic('driver', path)
driver.pyrun(DecafSizes, prod_nsteps, infile)
