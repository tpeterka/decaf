# include the following 3 lines each time
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

# path to python run .so
run_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mpmd/python/py_linear_2nodes_prod.so'

# define workflow graph
import linear_2nodes_no_overlap as gr
w = gr.make_graph()

# convert the nx graph into a workflow data structure and run the workflow
wf.workflow(w, run_path)

