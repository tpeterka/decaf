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
# prod(2 procs)    prod2(2 procs)
#             \   /
#			   \ /
#           con (2 procs)
# entire workflow takes 8 procs (1 proc per link)

# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod", "prod2", "con", "dflow1", "dflow2"],[1,1,2,1,1])


prod = wf.nodeFromTopo("prod", "prod", "ports", subtopos[0])
prod.addOutPort("Out")
prod.addOutput("Out", "value", "Vector_int")

prod2 = wf.nodeFromTopo("prod2", "prod2", "ports", subtopos[1])
prod2.addOutPort("Out")
prod2.addOutput("Out", "value", "Array_int")

con = wf.nodeFromTopo("con", "con", "ports", subtopos[2])
con.addInPort("In1")
con.addInput("In1", "value", "Vector_int")
con.addInPort("In2")
con.addInput("In2", "value", "Array_int")


w = nx.DiGraph()
wf.addNode(w, prod)
wf.addNode(w, prod2)
wf.addNode(w, con)

wf.addEdgeWithTopo(w, "prod.Out", "con.In1", subtopos[3], 'count', 'dflow', mod_path, 'count', 'ports')
wf.addEdgeWithTopo(w, "prod2.Out", "con.In2", subtopos[4], 'count', 'dflow', mod_path, 'count', 'ports')

"""
# TODO THIS is for testing an out connected to multiple in
con2 = wf.Node("con2", 4, 1, "con2", "ports")
con2.addInPort("In")
wf.addNode(w, con2)
wf.addEdge(w, "prod2.Out", "con2.In", 4, 0, 'count')
"""
# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "ports", check_types = 2)
