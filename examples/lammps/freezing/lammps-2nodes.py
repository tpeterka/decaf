# a small 2-node example

# input file
infile = 'in.watbox'

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/lammps/melting/mod_lammps.so'


# define workflow graph

# 2-node workflow
#
#   lammps (4 procs) -> filter (4 procs)
#

# --- Graph definition ---
lammps   = wf.Node("lammps", start_proc=0, nprocs=4, func='lammps', cmdline='./lammps')
outPort0 = lammps.addOutputPort("out")

detect   = wf.Node("detect", start_proc=4, nprocs=4, func='detect', cmdline='./detect')
inPort1  = detect.addInputPort("in")

lammps_detect = wf.Edge(lammps.getOutputPort("out"), detect.getInputPort("in"),
        start_proc=0,  nprocs=0, func='', path=mod_path, prod_dflow_redist='count',
        dflow_con_redist='count', cmdline='./lammps')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("lammps",infile)
