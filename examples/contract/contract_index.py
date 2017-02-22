# An example with two different producers and two different consumers to test the contracts

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_contract_index.so'

# define workflow graph
# prod (1 proc)  ---> con (1 proc) 
#				  \/
#				  /\
# prod2 (1 proc) ---> con2 (1 proc)
#
# entire workflow takes 8 procs (1 proc per link)

# Creating the topology
topo = wf.Topology("topo", 17)
subtopos = topo.splitTopology(["prod1", "prod2", "con1", "con2", "dflow11", "dflow12", "dflow21", "dflow22"],[2,1,3,1,4,2,3,1])

# Creating Node objects
P1 = wf.nodeFromTopo("prod1", "prod", "contract", subtopos[0])
contractP1 = wf.Contract()
contractP1.addOutputFromDict({"index":["int", 1], "velocity":["Array_float", 2]})
P1.addContract(contractP1)

P2 = wf.nodeFromTopo("prod2", "prod2", "contract", subtopos[1])
contractP2 = wf.Contract()
contractP2.addOutputFromDict({"vel":["Array_float"], "id":["int"], "density":["Array_float", 3]})
P2.addContract(contractP2)

C1 = wf.nodeFromTopo("con1", "con", "contract", subtopos[2])
contractC1 = wf.Contract()
contractC1.addInputFromDict({"index":["int"], "velocity":["Array_float", 2], "density":["Array_float"]})
C1.addContract(contractC1)

C2 = wf.nodeFromTopo("con2", "con2", "contract", subtopos[3])
contractC2 = wf.Contract()
contractC2.addInputFromDict({"velocity":["Array_float", 2], "id":["int"]})
C2.addContract(contractC2)

# Creating Edge objects
edge11 = wf.edgeFromTopo("prod1", "con1", subtopos[4], 'count', 'dflow', mod_path, 'count', 'contract_index')
edge12 = wf.edgeFromTopo("prod1", "con2", subtopos[5], 'count', 'dflow', mod_path, 'count', 'contract_index')
edge21 = wf.edgeFromTopo("prod2", "con1", subtopos[6], 'count', 'dflow', mod_path, 'count', 'contract_index')
edge22 = wf.edgeFromTopo("prod2", "con2", subtopos[7], 'count', 'dflow', mod_path, 'count', 'contract_index')


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
wf.processGraph(graph, "contract_index", filter_level = wf.Filter_level.PY_AND_SOURCE)