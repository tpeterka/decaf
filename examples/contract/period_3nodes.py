# An example with two producers and one consumer to test the ports

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
mod_path = os.environ['DECAF_PREFIX'] + '/examples/ports/mod_ports.so'


# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["node1", "node2", "node3", "dflow1", "dflow2"],[1,1,1,0,0])

contractA = wf.Contract()
contractA.addOutputFromDict({"var":["int", 2]})

contractB = wf.Contract()
contractB.addOutputFromDict({"var":["int"]})
contractB.addInput("var", "int")

contractC = wf.Contract()
contractC.addInputFromDict({"var":["int"]})


w = nx.DiGraph()
w.add_node("node1", topology=subtopos[0], contract=contractA, func='node1', cmdline='period_3nodes')
w.add_node("node2", topology=subtopos[1], contract=contractB, func='node2', cmdline='period_3nodes')
w.add_node("node3",  topology=subtopos[2], contract=contractC, func='node3', cmdline='period_3nodes')
w.add_edge("node1", "node2", topology=subtopos[3], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='period_3nodes')
w.add_edge("node2", "node3", topology=subtopos[4], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='period_3nodes')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "period_3nodes", check_types = wf.Check_types.PYTHON)