# An example with two producers and one consumer to test the ports

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/ports/mod_ports.so'

# define workflow graph
# prod(1 proc)    prod2(1 proc)
#             \   /
#			   \ /
#           con (2 procs)
# entire workflow takes 6 procs (1 proc per link)

# Creating the topology
topo = wf.Topology("topo", 16)
subtopos = topo.splitTopology(["prod", "prod2", "con", "dflow1", "dflow2"],[1,1,2,1,1])


prod = wf.nodeFromTopo("prod", "prod", "ports", subtopos[0])
prod.addOutput("Out", "value", "int", 1)

prod2 = wf.nodeFromTopo("prod2", "prod2", "ports", subtopos[1])
prod2.addOutput("Out", "value", "int", 1)

con = wf.nodeFromTopo("con", "con", "ports", subtopos[2])
con.addInput("In1", "value", "int", 1)
con.addInput("In2", "value", "int", 2)

edge1 = wf.edgeFromTopo("prod.Out", "con.In1", subtopos[3], 'count', 'dflow', mod_path, 'count', 'ports')
edge2 = wf.edgeFromTopo("prod2.Out", "con.In2", subtopos[4], 'count', 'dflow', mod_path, 'count', 'ports')


graph = nx.DiGraph()
wf.addNode(graph, prod)
wf.addNode(graph, prod2)
wf.addNode(graph, con)

wf.addEdge(graph, edge1)
wf.addEdge(graph, edge2)


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(graph, "ports", filter_level = wf.Filter_level.EVERYWHERE)