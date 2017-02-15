# An example with two different producers and two different consumers to test the contracts

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_contract_ports.so'

# define workflow graph
# prod (1 proc)  ---> con (1 proc) 
#				  \/
#				  /\
# prod2 (1 proc) ---> con2 (1 proc)
#
# entire workflow takes 4 procs (0 proc per link)


# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod1", "prod2", "con1", "con2", "dflow11", "dflow12", "dflow21", "dflow22"],[1,1,1,1,0,0,0,0])


prod1 = wf.nodeFromTopo("prod1", "prod", "contract_ports", subtopos[0])
prod1.addOutputFromDict("Out", {"index":["int"], "velocity":["Array_float", 2]})

prod2 = wf.nodeFromTopo("prod2", "prod2", "contract_ports", subtopos[1])
prod2.addOutput("Out1", "density", "Array_float", 3)
prod2.addOutput("Out2", "id", "int")

con1 = wf.nodeFromTopo("con1", "con", "contract_ports", subtopos[2])
con1.addInputFromDict("In1", {"index":["int"], "velocity":["Array_float", 2]})
con1.addInput("In2", "density", "Array_float")

con2 = wf.nodeFromTopo("con2", "con2", "contract_ports", subtopos[3])
con2.addInput("In1", "velocity","Array_float", 2)
con2.addInput("In2", "id", "int")

edge11 = wf.edgeFromTopo("prod1.Out", "con1.In1", subtopos[4], 'count')
edge12 = wf.edgeFromTopo("prod1.Out", "con2.In1", subtopos[5], 'count')
edge21 = wf.edgeFromTopo("prod2.Out1", "con1.In2", subtopos[6], 'count')
edge22 = wf.edgeFromTopo("prod2.Out2", "con2.In2", subtopos[7], 'count')


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
wf.processGraph(w, "contract_ports", filter_level = wf.Filter_level.PY_AND_SOURCE)