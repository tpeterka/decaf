# A simple example with two nodes and a link to test contracts on nodes and links
# to test different means of using contracts and show the errors/warnings
# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_contract_link.so'

# Creating the topology
topo = wf.Topology("topo", 8)
subtopos = topo.splitTopology(["prod", "con", "dflow"], [1,1,1])

# Creating the nodes
prod = wf.nodeFromTopo("prod", "prod", "./contract_link", subtopos[0])
prod.addOutput("Out", "var", "float")
prod.addOutput("Out", "toto", "int")

con = wf.nodeFromTopo("con", "con", "./contract_link", subtopos[1])
con.addInput("In", "var", "int")
con.addInput("In", "toto", "int")

# Creating the edge
edge = wf.edgeFromTopo("prod.Out", "con.In", subtopos[2], 'count', 'dflow', mod_path, 'count', './contract_link')
clink = wf.ContractLink(bAny = True) # When bAny set to False, the field 'toto' should not be sent/received at runtime
clink.addInput("var", "float", 1)
clink.addOutput("var", "int", 1)
edge.addContractLink(clink)

graph = nx.DiGraph()
wf.addNode(graph, prod)
wf.addNode(graph, con)
wf.addEdge(graph, edge)

# --- convert the nx graph into a worflow data structure and performs all the checks ---
wf.processGraph(graph, "contract_link", filter_level=wf.Filter_level.PY_AND_SOURCE)
