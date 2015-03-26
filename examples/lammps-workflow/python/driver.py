import networkx as nx

# --- set your options here ---

# path to .so module
path = '/Users/tpeterka/software/decaf/install/examples/lammps-workflow/python/libpy_lammps_workflow.so'

# define workflow graph
w = nx.DiGraph()
w.add_nodes_from(["l", "p1", "p2", "p3"])
w.add_edges_from([("l", "p1"), ("l", "p2"), ("p2", "p3")])
w.nodes()
w.degree()

# # communicator sizes and time steps
# class DecafSizes:
#     prod_size    = 4      # size of producer communicator
#     dflow_size   = 2      # size of dataflow communicator
#     con_size     = 2      # size of consumer communicator
#     prod_start   = 0      # starting rank of producer communicator in the world
#     dflow_start  = 4      # starting rank of dataflow communicator in the world
#     con_start    = 6      # starting rank of consumer communicator in the world
#     con_nsteps   = 1      # number of consumer (decaf) time steps

# # total number of time steps; this is the user's variable, not part of decaf
# prod_nsteps  = 1

# # lammps input file
# infile = "/Users/tpeterka/software/decaf/examples/lammps-workflow/in.melt"

# # --- do not edit below this point --

# import imp
# driver = imp.load_dynamic('driver', path)
# driver.pyrun(DecafSizes, prod_nsteps, infile)
