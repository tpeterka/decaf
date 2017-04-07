# An example with two different producers and two different consumers to test the contracts

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')


# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_contract.so'

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


prod1 = wf.nodeFromTopo("prod1", "prod", "./contract", subtopos[0])
prod1.addOutputFromDict("Out", {"index":["int"], "velocity":["Array_float", 2]})

prod2 = wf.nodeFromTopo("prod2", "prod2", "./contract", subtopos[1])
prod2.addOutputFromDict("Out", {"id":["int"], "density":["Array_float", 3]})

con1 = wf.nodeFromTopo("con1", "con", "./contract", subtopos[2])
con1.addInputFromDict("In1", {"index":["int"], "velocity":["Array_float", 2]})
con1.addInput("In2", "density", "Array_float")

con2 = wf.nodeFromTopo("con2", "con2", "./contract", subtopos[3])
con2.addInput("In1", "velocity","Array_float", 2)
con2.addInput("In2", "id", "int")

edge11 = wf.edgeFromTopo("prod1.Out", "con1.In1", subtopos[4], 'count', 'dflow', mod_path, 'count', 'contract')
edge12 = wf.edgeFromTopo("prod1.Out", "con2.In1", subtopos[5], 'count', 'dflow', mod_path, 'count', 'contract')
edge21 = wf.edgeFromTopo("prod2.Out", "con1.In2", subtopos[6], 'count', 'dflow', mod_path, 'count', 'contract')
edge22 = wf.edgeFromTopo("prod2.Out", "con2.In2", subtopos[7], 'count', 'dflow', mod_path, 'count', 'contract')


w = nx.DiGraph()

wf.addNode(w, prod1)
wf.addNode(w, prod2)
wf.addNode(w, con1)
wf.addNode(w, con2)

wf.addEdge(w, edge11)
wf.addEdge(w, edge12)
wf.addEdge(w, edge21)
wf.addEdge(w, edge22)


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "contract", filter_level = wf.Filter_level.PY_AND_SOURCE)
