# An example with one producer and one consumer to test the contract on link and periodicity

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_period_link.so'


# Creating the topology
topo = wf.Topology("topo", 8)
subtopos = topo.splitTopology(["prod", "con", "dflow"],[1,1,1])

prod = wf.nodeFromTopo("prod", "prod", "./period_link", subtopos[0])
prod.addOutput("Out","var", "float", 2)

con = wf.nodeFromTopo("con", "con", "./period_link", subtopos[1])
con.addInput("In", "var", "int", 1)

edge = wf.edgeFromTopo("prod.Out", "con.In", subtopos[2], 'count', 'dflow', mod_path, 'count', './period_link')
clink = wf.ContractLink(bAny = True)
clink.addInput("var", "float", 1)
clink.addOutput("var", "int", 1)
edge.addContractLink(clink)


graph = nx.DiGraph()
wf.addNode(graph, prod)
wf.addNode(graph, con)
wf.addEdge(graph, edge)


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(graph, "period_link", filter_level = wf.Filter_level.PY_AND_SOURCE)
