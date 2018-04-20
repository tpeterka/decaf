# a small 2-node example
# usage: python3 lammps-2nodes.py --args "<infile> <outfile>"

# --- include the following lines each time ---

import networkx as nx
import os
import imp
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# parse command line args
parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# define workflow graph

# 2-node workflow
#
#   lammps (4 procs) -> filter (4 procs)
#

# --- Graph definition ---
lammps   = wf.Node("lammps", start_proc=0, nprocs=4, func='lammps', cmdline='./freeze')
outPort0 = lammps.addOutputPort("out")

detect   = wf.Node("detect", start_proc=4, nprocs=4, func='detect', cmdline='./detect')
inPort1  = detect.addInputPort("in")

lammps_detect = wf.Edge(lammps.getOutputPort("out"), detect.getInputPort("in"),
        start_proc=0,  nprocs=0, func='', prod_dflow_redist='count', dflow_con_redist='count')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("lammps", args.custom_args)

