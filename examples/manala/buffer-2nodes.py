# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
import sys
import argparse

wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/buffer/mod_dflow_buffer.so'

# define workflow graph
# 2-node workflow
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
wf.initParserForTopology(parser)
args = parser.parse_args()

# Creating the topology
topo = wf.topologyFromArgs(args)
subtopos = topo.splitTopologyByDict([{'name':'prod', 'nprocs':2},{'name':'dflow', 'nprocs':2},{'name':'con', 'nprocs':1}])

# Creating the graph
#w = nx.DiGraph()
#w.add_node("prod", topology=subtopos[0], func='prod', cmdline='./prod_buffer')
prod = wf.Node("prod", topology=subtopos['prod'], func='prod', cmdline='./prod_buffer')
outPort = prod.addOutputPort("out")


#w.add_node("con", topology=subtopos[2], func='con', cmdline='./con_buffer')
con = wf.Node("con", topology=subtopos['con'], func='con', cmdline='./con_buffer')
inPort = con.addInputPort("in")

#w.add_edge("prod", "con", topology=subtopos[1], func='dflow', path=mod_path,
#           prod_dflow_redist='count', dflow_con_redist='count', cmdline='./dflow_buffer')
link = wf.Edge(prod.getOutputPort("out"), con.getInputPort("in"), topology=subtopos['dflow'], func='dflow', path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./dflow_buffer')

#link.addSeqStream(buffers = ["mainmem"], max_buffer_sizes = [3])
#link.addMostRecentStream( buffers = ["mainmem"], max_buffer_sizes = [3])
#link.addSeqStream( prod_output_freq = 2)
link.addLowHighStream( 3, 1, buffers = ["mainmem"], max_buffer_sizes = [10])


# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph( "buffer")
