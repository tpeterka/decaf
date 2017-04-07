# a small 3-node example, synthetic point generation, tessellation, and density estimation

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/tess_dense/mod_pts_dflow.so'

# define workflow graph
# 3-node workflow
#
# points (8 procs) -> dflow1 (4 procs) -> tesselate (2 procs) -> dflow2 (2 procs) ->
# density_estimate (2 procs)
#
#  entire workflow takes 18 procs


parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod","dflow","tess","dense"],[4,4,2,1])

tot_blocks = 4
tot_particles = 80

w = nx.DiGraph()
w.add_node("prod",                           topology=subtopos[0], func='prod',
           cmdline='./points '+str(tot_particles))
w.add_node("tessellate",                     topology=subtopos[2], func='tessellate',
           cmdline='./tess '+str(tot_blocks))
w.add_node("density_estimate",               topology=subtopos[3], func='density_estimate',
           cmdline='./dense '+str(tot_blocks))
w.add_edge("prod", "tessellate",             topology=subtopos[1], func='dflow1',
           path=mod_path, prod_dflow_redist='proc', dflow_con_redist='proc', cmdline='./pts_dflow')
w.add_edge("tessellate", "density_estimate", start_proc=0,  nprocs=0, func='dflow2',
           path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./pts_dflow')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "tess_dense")
