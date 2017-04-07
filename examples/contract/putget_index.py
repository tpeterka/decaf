# An example with two different producers and two different consumers to test the put and get on a given dataflow index

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_putget_index.so'

# define workflow graph
# prod (1 proc)  ---> con (1 proc) 
#				  \/
#				  /\
# prod2 (1 proc) ---> con2 (1 proc)
#
# entire workflow takes 8 procs (1 proc per link)

# Creating the topology
topo = wf.Topology("topo", 16)
subtopos = topo.splitTopology(["prod1", "prod2", "con1", "con2", "dflow11", "dflow12", "dflow21", "dflow22"],[1,1,1,1,1,1,1,1])

# Creating Node objects
P1 = wf.nodeFromTopo("prod1", "prod", "./putget_index", subtopos[0])

P2 = wf.nodeFromTopo("prod2", "prod2", "./putget_index", subtopos[1])

C1 = wf.nodeFromTopo("con1", "con", "./putget_index", subtopos[2])

C2 = wf.nodeFromTopo("con2", "con2", "./putget_index", subtopos[3])

# Creating Edge objects
edge11 = wf.edgeFromTopo("prod1", "con1", subtopos[4], 'count', 'dflow', mod_path, 'count', './putget_index')
edge12 = wf.edgeFromTopo("prod1", "con2", subtopos[5], 'count', 'dflow', mod_path, 'count', './putget_index')
edge21 = wf.edgeFromTopo("prod2", "con1", subtopos[6], 'count', 'dflow', mod_path, 'count', './putget_index')
edge22 = wf.edgeFromTopo("prod2", "con2", subtopos[7], 'count', 'dflow', mod_path, 'count', './putget_index')


graph = nx.DiGraph()
wf.addNode(graph, P1)
wf.addNode(graph, P2)
wf.addNode(graph, C1)
wf.addNode(graph, C2)
wf.addEdge(graph, edge11)
wf.addEdge(graph, edge12)
wf.addEdge(graph, edge21)
wf.addEdge(graph, edge22)

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(graph, "putget_index", filter_level = wf.Filter_level.PY_AND_SOURCE)
