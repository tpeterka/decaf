# a small 2-node example, just a producer and consumer

# --- include the following 5 lines each time ---

import networkx as nx
import os
import imp
import argparse
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_2nodes.so'

# --- topology declaration ---
parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# Creating the topology
topo = wf.topologyFromArgs(args)

subtopos = topo.splitTopologyByDict([{"name":"prod", "nprocs":4},{"name":"dflow", "nprocs":2},{"name":"con", "nprocs":2}])

# --- Contract definitions ---
prodContract = wf.Contract()
prodContract.addEntry('var', 'int', 1)

conContract = wf.Contract()
conContract.addEntry('var', 'int', 1)

linkContract = wf.ContractLink(True)

# define workflow graph
# 2-node workflow
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

# --- Graph definition ---
prod = wf.Node("prod", topology = subtopos['prod'], func='prod', cmdline='./linear_2nodes')
outPort = prod.addOutputPort("out")
outPort.setContract(prodContract)

con = wf.Node("con", topology = subtopos['con'], func='con', cmdline='./linear_2nodes')
inPort = con.addInputPort("in")
inPort.setContract(conContract)

link = wf.Edge(prod.getOutputPort("out"), con.getInputPort("in"), topology = subtopos['dflow'], func='dflow',
        path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./linear_2nodes')
link.setContractLink(linkContract)

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("linear2", filter_level = wf.Filter_level.PYTHON)
