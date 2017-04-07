# An example with 3 nodes to test the periodicity of fields

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_period_3nodes.so'


# Creating the topology
topo = wf.Topology("topo", 16)
subtopos = topo.splitTopology(["node1", "node2", "node3", "dflow1", "dflow2"],[1,1,1,0,0])

node1 = wf.nodeFromTopo("node1", "node1", "./period_3nodes", subtopos[0])
node1.addOutputFromDict("Out", {"var":["int", 2]})

node2 = wf.nodeFromTopo("node2", "node2", "./period_3nodes", subtopos[1])
node2.addOutput("Out", "var", "int")
node2.addInput("In", "var", "int")

node3 = wf.nodeFromTopo("node3", "node3", "./period_3nodes", subtopos[2])
node3.addInput("In", "var", "int")


edge12 = wf.edgeFromTopo("node1.Out", "node2.In", subtopos[3], 'count', 'dlfow', mod_path, 'count', './period_3nodes')
edge23 = wf.edgeFromTopo("node2.Out", "node3.In", subtopos[4], 'count', 'dlfow', mod_path, 'count', './period_3nodes')


graph = nx.DiGraph()
wf.addNode(graph, node1)
wf.addNode(graph, node2)
wf.addNode(graph, node3)
wf.addEdge(graph, edge12)
wf.addEdge(graph, edge23)


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(graph, "period_3nodes", filter_level = wf.Filter_level.PY_AND_SOURCE)
