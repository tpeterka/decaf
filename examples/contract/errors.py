# A simple example with two nodes
# to test typechecking with different scenarios

# Simple example, with 1 producer, 1 consumer and one dataflow between the two
# Prod (1 proc) --> Con (1 proc)

# --- include the following 4 lines each time ---
import os
import imp
import networkx as nx
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_errors.so'

# Creating the topology
topo = wf.Topology("topo", 8)
subtopos = topo.splitTopology(["prod", "con", "dflow"], [1,1,1])


# --- SCENARIO 1 ---
# The name of the data in the contracts of prod.Out and con.In are different. ERROR during python script execution: field "value" is not in the contract of prod.Out
# Creating the nodes
prod = wf.nodeFromTopo("prod", "prod", "./errors", subtopos[0])
prod.addOutput("Out", "var", "Vector_int")

con = wf.nodeFromTopo("con", "con", "./errors", subtopos[1])
con.addInput("In", "value", "Vector_int")

# Creating the edge
edge = wf.edgeFromTopo("prod.Out", "con.In", subtopos[2], 'count', 'dflow', mod_path, 'count', './errors')
# --- END 1 ---

"""
# --- SCENARIO 2 ---
# The name of the data in the contracts of prod.Out and con.In are equal, but the types are different. ERROR during python script execution
# Creating the nodes
prod = wf.nodeFromTopo("prod", "prod", "./errors", subtopos[0])
prod.addOutput("Out", "var", "Array_int")

con = wf.nodeFromTopo("con", "con", "./errors", subtopos[1])
con.addInput("In", "var", "Vector_int")

# Creating the edge
edge = wf.edgeFromTopo("prod.Out", "con.In", subtopos[2], 'count', 'dflow', mod_path, 'count', './errors')
# --- END 2 ---
"""
"""
# --- SCENARIO 3 ---
# The name and type of the data in the contracts of prod.Out and con.In are equal. Python script is ok.
# The ERROR occurs during the runtime, when the producer does not send "var" with the correct type
# Creating the nodes
prod = wf.nodeFromTopo("prod", "prod", "./errors", subtopos[0])
prod.addOutput("Out", "var", "Vector_int")

con = wf.nodeFromTopo("con", "con", "./errors", subtopos[1])
con.addInput("In", "var", "Vector_int")

# Creating the edge
edge = wf.edgeFromTopo("prod.Out", "con.In", subtopos[2], 'count', 'dflow', mod_path, 'count', './errors')
# --- END 3 ---
"""


graph = nx.DiGraph()
wf.addNode(graph, prod)
wf.addNode(graph, con)
wf.addEdge(graph, edge)

# --- convert the nx graph into a worflow data structure and performs all the checks ---
wf.processGraph(graph, "errors", filter_level=wf.Filter_level.PY_AND_SOURCE)
