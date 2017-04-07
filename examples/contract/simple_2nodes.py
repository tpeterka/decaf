# A simple example with two nodes using contracts
# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_simple_2nodes.so'

# Creating the topology
topo = wf.Topology("topo", 8)
subtopos = topo.splitTopology(["prod", "con", "dflow"], [1,1,1])

# Creating the nodes
prod = wf.nodeFromTopo("prod", "prod", "./simple_2nodes", subtopos[0])
prod.addOutputPort("Out")

con = wf.nodeFromTopo("con", "con", "./simple_2nodes", subtopos[1])
con.addInput("In", "var", "int")

# Creating the edge
edge = wf.edgeFromTopo("prod.Out", "con.In", subtopos[2], 'count', 'dflow', mod_path, 'count', './simple_2nodes')

graph = nx.DiGraph()
wf.addNode(graph, prod)
wf.addNode(graph, con)
wf.addEdge(graph, edge)

# --- convert the nx graph into a worflow data structure and performs all the checks ---
wf.processGraph(graph, "simple_2nodes", filter_level=wf.Filter_level.PY_AND_SOURCE)
