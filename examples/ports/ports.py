# An example with two producers and one consumer to test the ports

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/ports/mod_ports.so'

# define workflow graph
# prod(1 proc)    prod2(1 proc)
#             \   /
#			   \ /
#           con (2 procs)
# entire workflow takes 6 procs (1 proc per link)

# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod", "prod2", "con", "dflow1", "dflow2"],[1,1,1,1,1])


prod = wf.nodeFromTopo("prod", "prod", "ports", subtopos[0])
prod2 = wf.nodeFromTopo("prod2", "prod2", "ports", subtopos[1])
con = wf.nodeFromTopo("con", "con", "ports", subtopos[2])

Cedge0 = wf.ContractEdge({"Index0":["int", 1]})
Cedge1 = wf.ContractEdge({"Index1":["int", 1]})


w = nx.DiGraph()
wf.addNode(w, prod)
wf.addNode(w, prod2)
wf.addNode(w, con)

w.add_edge("prod", "con", edge_id=0, topology=subtopos[3], contract=Cedge0, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='ports')
w.add_edge("prod2", "con", edge_id=1, topology=subtopos[4], contract=Cedge1, func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='ports')


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "ports", filter_level = wf.Filter_level.EVERYWHERE)
