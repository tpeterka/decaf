# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mpmd/mod_dflow.so'


# define workflow graph
# 2-node workflow
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

# --- Graph definition ---
prod = wf.Node("prod", start_proc=0, nprocs=1, func='prod', cmdline='python3 ./prod.py')
outPort = prod.addOutputPort("out")

con = wf.Node("con", start_proc=1, nprocs=1, func='con', cmdline='python3 ./con.py')
inPort = con.addInputPort("in")

link = wf.Edge(prod.getOutputPort("out"), con.getInputPort("in"), start_proc=0, nprocs=0, func='dflow',
        path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./dflow')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("linear2")
