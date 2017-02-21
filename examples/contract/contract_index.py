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
topo = wf.Topology("topo", 16)
subtopos = topo.splitTopology(["prod1", "prod2", "con1", "con2", "dflow11", "dflow12", "dflow21", "dflow22"],[1,1,1,1,1,1,1,1])

# Add outputs contracts for prod1 and prod2
contractP1 = wf.Contract()
contractP1.addOutputFromDict({"index":["int", 1], "velocity":["Array_float", 2]})
contractP2 = wf.Contract()
contractP2.addOutputFromDict({"vel":["Array_float"], "id":["int"], "density":["Array_float", 3]})
# Add inputs contracts for con1 and con2
contractC1 = wf.Contract()
contractC1.addInputFromDict({"index":["int"], "velocity":["Array_float", 2], "density":["Array_float"]})
contractC2 = wf.Contract()
contractC2.addInputFromDict({"velocity":["Array_float", 2], "id":["int"]})


w = nx.DiGraph()
w.add_node("prod1", topology=subtopos[0], contract=contractP1, func='prod', cmdline='contract_index')
w.add_node("con1",  topology=subtopos[2], contract=contractC1, func='con', cmdline='contract_index')

w.add_node("prod2", topology=subtopos[1], contract=contractP2, func='prod2', cmdline='contract_index')
w.add_node("con2",  topology=subtopos[3], contract=contractC2, func='con2', cmdline='contract_index')

w.add_edge("prod1", "con1", topology=subtopos[4], prod_dflow_redist='count', dflow_con_redist='count', func='dflow', path=mod_path, cmdline='contract_index')
w.add_edge("prod1", "con2", topology=subtopos[5], prod_dflow_redist='count', dflow_con_redist='count', func='dflow', path=mod_path, cmdline='contract_index')
w.add_edge("prod2", "con1", topology=subtopos[6], prod_dflow_redist='count', dflow_con_redist='count', func='dflow', path=mod_path, cmdline='contract_index')
w.add_edge("prod2", "con2", topology=subtopos[7], prod_dflow_redist='count', dflow_con_redist='count', func='dflow', path=mod_path, cmdline='contract_index')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph(w, "contract_index", filter_level = wf.Filter_level.PY_AND_SOURCE)