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
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_my_test.so'

# define workflow graph
# TODO little schema graph
#  entire workflow takes 16 procs
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod1", "prod2", "dflow11", "con1", "con2", "dflow21", "dflow12", "dflow22"],[2,2,2,2,2,2,2,2])

# Add outputs contracts for prod1 and prod2
subtopos[0].addOutputDict({"index":"int", "velocity":"float"})
subtopos[1].addOutputDict({"density":"float", "vel":"float", "id":"int"})
# Add inputs contracts for con1 and con2
subtopos[3].addInputDict({"index":"int", "velocity":"float", "density":"float"})
subtopos[4].addInputDict({"velocity":"float", "id":"int"})

topo.addOutputDict

w = nx.DiGraph()
w.add_node("prod1", topology=subtopos[0], func='prod', cmdline='my_test')
w.add_node("con1",  topology=subtopos[3], func='con', cmdline='my_test')
w.add_edge("prod1", "con1", topology=subtopos[2], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='my_test')

w.add_node("prod2", topology=subtopos[1], func='prod2', cmdline='my_test')
w.add_node("con2",  topology=subtopos[4], func='con2', cmdline='my_test')
w.add_edge("prod2", "con1", topology=subtopos[5], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='my_test')
w.add_edge("prod1", "con2", topology=subtopos[6], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='my_test')
w.add_edge("prod2", "con2", topology=subtopos[7], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='my_test')


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "my_test", mod_path)
