# a small 3-node example, synthetic point generation, tessellation, and density estimation
# usage: python3 tess_dense.py --args "<tot_npts> <nsteps> <nblocks> <outfile>"

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/tess_dense/mod_pts_dflow.so'

# parse command line args
parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# create the topology
topo     = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["pts","dflow","tess","dense"],[4,4,2,1])

# tot_pts     = 80
# nsteps      = 3
# tot_blocks  = 4
# outfile     = "dense"

# define the graph
pts         = wf.Node("points", topology=subtopos[0], func='prod', cmdline='./points')
pts_out     = pts.addOutputPort("pts_out")

tess        = wf.Node("tessellate", topology=subtopos[2], func='tessellate', cmdline='./tess')
tess_in     = tess.addInputPort("tess_in")
tess_out    = tess.addOutputPort("tess_out")

dense       = wf.Node("density_estimate", topology=subtopos[3], func='density_estimate', cmdline='./dense')
dense_in    = dense.addInputPort("dense_in")

pts_tess    = wf.Edge(pts.getOutputPort("pts_out"), tess.getInputPort("tess_in"),
        topology=subtopos[1], func='dflow1', path=mod_path, prod_dflow_redist='proc', dflow_con_redist='proc', cmdline='./pts_dflow')

tess_dense  = wf.Edge(tess.getOutputPort("tess_out"), dense.getInputPort("dense_in"),
        start_proc=0,  nprocs=0, func='', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./pts_dflow')

# --- convert the graph into a workflow ---
wf.processGraph("tess_dense")
