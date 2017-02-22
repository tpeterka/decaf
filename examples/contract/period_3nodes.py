# An example with two producers and one consumer to test the ports

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

node1 = wf.nodeFromTopo("node1", "node1", "period_3nodes", subtopos[0])
contractA = wf.Contract()
contractA.addOutputFromDict({"var":["int", 2]})
node1.addContract(contractA)

node2 = wf.nodeFromTopo("node2", "node2", "period_3nodes", subtopos[1])
contractB = wf.Contract()
contractB.addOutputFromDict({"var":["int"]})
contractB.addInput("var", "int")
node2.addContract(contractB)

node3 = wf.nodeFromTopo("node3", "node3", "period_3nodes", subtopos[2])
contractC = wf.Contract()
contractC.addInputFromDict({"var":["int"]})
node3.addContract(contractC)

edge12 = wf.edgeFromTopo("node1", "node2", subtopos[3], 'count', 'dlfow', mod_path, 'count', 'period_3nodes')

edge23 = wf.edgeFromTopo("node2", "node3", subtopos[4], 'count', 'dlfow', mod_path, 'count', 'period_3nodes')


graph = nx.DiGraph()
wf.addNode(graph, node1)
wf.addNode(graph, node2)
wf.addNode(graph, node3)
wf.addEdge(graph, edge12)
wf.addEdge(graph, edge23)


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(graph, "period_3nodes", filter_level = wf.Filter_level.PY_AND_SOURCE)