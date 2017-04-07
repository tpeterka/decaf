# An example with two different producers and one consumer to test the contracts on links

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')


# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_conversion.so'

# define workflow graph
# prod (1 proc) ---> con (1 proc) 
#				  /
#				 /
# prod2 (1 proc) 
#
# entire workflow takes 5 procs (1 proc per link)


# Creating the topology
topo = wf.Topology("topo", 16)
subtopos = topo.splitTopology(["prod1", "prod2", "con", "dflow1", "dflow2"],[1,1,1,1,1])
# TODO play with the link with 0 nprocs, for dflow1 should be ok but not dflow2 since the contract b/w producer2 and consumer1 would not match


prod1 = wf.nodeFromTopo("prod1", "prod", "./conversion", subtopos[0])
prod1.addOutputFromDict("Out", {"vector":["Vector_int", 1]})

prod2 = wf.nodeFromTopo("prod2", "prod2", "./conversion", subtopos[1])
prod2.addOutput("Out", "array", "Array_int")

con1 = wf.nodeFromTopo("con1", "con", "./conversion", subtopos[2])
con1.addInput("In1", "vector", "Vector_int")
con1.addInput("In2", "converted", "Vector_int")


edge1 = wf.edgeFromTopo("prod1.Out", "con1.In1", subtopos[3], 'count', 'dflow1', mod_path, 'count', './conversion')
edge2 = wf.edgeFromTopo("prod2.Out", "con1.In2", subtopos[4], 'count', 'dflow2', mod_path, 'count', './conversion')
cLink = wf.ContractLink(bAny = False)
cLink.addInput("array", "Array_int")
cLink.addOutput("converted", "Vector_int")
edge2.addContractLink(cLink)


graph = nx.DiGraph()

wf.addNode(graph, prod1)
wf.addNode(graph, prod2)
wf.addNode(graph, con1)

wf.addEdge(graph, edge1)
wf.addEdge(graph, edge2)

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(graph, "conversion", filter_level = wf.Filter_level.PY_AND_SOURCE)
