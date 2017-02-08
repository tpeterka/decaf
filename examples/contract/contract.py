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
mod_path = os.environ['DECAF_PREFIX'] + '/examples/contract/mod_contract.so'

# define workflow graph
# prod (3 procs)  ---> con (2 procs) 
#				  \/
#				  /\
# prod2 (2 procs) ---> con2 (2 procs)
#
# entire workflow takes 13 procs (1 proc per link)



# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopology(["prod1", "prod2", "con1", "con2", "dflow11", "dflow12", "dflow21", "dflow22"],[1,1,1,1,0,0,0,0])

# Add outputs contracts for prod1 and prod2
contractP1 = wf.Contract()
contractP1.addOutputFromDict({"index":["int", 1], "velocity":["Array_float", 1]})
contractP2 = wf.Contract()
contractP2.addOutputFromDict({"vel":["Array_float"], "id":["int"], "density":["Array_float", 3]})
# Add inputs contracts for con1 and con2
contractC1 = wf.Contract()
contractC1.addInputFromDict({"index":["int"], "velocity":["Array_float", 2], "density":["Array_float"]})
contractC2 = wf.Contract()
contractC2.addInputFromDict({"velocity":["Array_float", 1], "id":["int", 2]})


w = nx.DiGraph()
w.add_node("prod1", topology=subtopos[0], contract=contractP1, func='prod', cmdline='contract')
w.add_node("con1",  topology=subtopos[2], contract=contractC1, func='con', cmdline='contract')
w.add_edge("prod1", "con1", topology=subtopos[4], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='contract')

w.add_node("prod2", topology=subtopos[1], contract=contractP2, func='prod2', cmdline='contract')
w.add_node("con2",  topology=subtopos[3], contract=contractC2, func='con2', cmdline='contract')

w.add_edge("prod1", "con2", topology=subtopos[5], prod_dflow_redist='count')
w.add_edge("prod2", "con1", topology=subtopos[6], prod_dflow_redist='count', func='dflow', path=mod_path, dflow_con_redist='count', cmdline='contract')
w.add_edge("prod2", "con2", topology=subtopos[7], prod_dflow_redist='count', func='dflow', path=mod_path, dflow_con_redist='count', cmdline='contract')


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "contract", check_types = 2)