# An example with one producer and one consumer to test the utilization of a user defined class

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
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mini_tests/mod_test_object.so'

# define workflow graph
# prod(3 procs) --> con(2 procs)
# link dflow (1 proc)
# entire workflow takes 6 procs

# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod", "dflow", "con"],[3,1,2])

# Add outputs contracts for prod
contractP = wf.Contract()
contractP.addOutputFromDict({"object":["Vector_My_class"]})
# Add inputs contracts for con
contractC = wf.Contract()
contractC.addInputFromDict({"object":["Vector_My_class"]})


w = nx.DiGraph()
w.add_node("prod", topology=subtopos[0], contract=contractP, func='prod', cmdline='test_object')
w.add_node("con",  topology=subtopos[2], contract=contractC, func='con', cmdline='test_object')
w.add_edge("prod", "con", topology=subtopos[1], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='test_object')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "test_object", filter_level = wf.Filter_level.PY_AND_SOURCE)