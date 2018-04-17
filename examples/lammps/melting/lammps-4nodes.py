# a small 4-node example

# input file
infile = 'in.melt'

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/lammps/melting/mod_lammps.so'

# define workflow graph

# 4-node workflow
#
#          print (1 proc)
#        /
#   lammps (4 procs)
#        \
#          print2 (1 proc) - print (1 proc)
#
#  entire workflow takes 10 procs (1 link proc between each producer consumer pair)
#

# --- Graph definition ---
lammps   = wf.Node("lammps", start_proc=0, nprocs=4, func='lammps', cmdline='./lammps')
outPort0 = lammps.addOutputPort("out")

print1   = wf.Node("print1", start_proc=5, nprocs=1, func='print', cmdline='./lammps')
inPort1  = print1.addInputPort("in")

print2   = wf.Node("print2", start_proc=7, nprocs=1, func='print2', cmdline='./lammps')
inPort2  = print2.addInputPort("in")
outPort2 = print2.addOutputPort("out")

print3   = wf.Node("print3", start_proc=9, nprocs=1, func='print', cmdline='./lammps')
inPort3  = print3.addInputPort("in")

link1    = wf.Edge(lammps.getOutputPort("out"), print1.getInputPort("in"), start_proc=4, nprocs=1, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./lammps')

link2    = wf.Edge(lammps.getOutputPort("out"), print2.getInputPort("in"), start_proc=6, nprocs=1, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./lammps')

link3    = wf.Edge(print2.getOutputPort("out"), print3.getInputPort("in"), start_proc=8, nprocs=1, func='dflow',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./lammps')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("lammps",infile)
