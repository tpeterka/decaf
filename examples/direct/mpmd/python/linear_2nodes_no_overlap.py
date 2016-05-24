# a small 2-node example, just a producer and consumer
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

# include the following 2 lines each time
import networkx as nx
import os

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mpmd/mod_dflow.so'

# define the workflow graph
def make_graph() :
    w = nx.DiGraph()
    w.add_node("prod", start_proc=0, nprocs=4, func='prod')
    w.add_node("con",  start_proc=6, nprocs=2, func='con')
    w.add_edge("prod", "con", start_proc=4, nprocs=2, func='dflow', path=mod_path,
               prod_dflow_redist='count', dflow_con_redist='count')
    return w

