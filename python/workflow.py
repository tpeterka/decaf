# converts an nx graph into a workflow data structure and runs the workflow

import imp

def workflow(graph,
             run_path,
             source_nodes=None):

    if source_nodes is None:
        source_nodes = []

    nodes   = []
    links   = []
    sources = []                                 # indices of source nodes

    # load the module
    # hard-coded name 'pymod' must match name of module in PYBIND11_PLUGIN in decaf.hpp
    mod = imp.load_dynamic('pymod', run_path)

    # iterate over nodes
    i = 0
    for node in graph.nodes_iter(data=True):
        wnode = mod.WorkflowNode(node[1]['start_proc'],
                                 node[1]['nprocs'],
                                 node[1]['func'],
                                 node[1]['path'])
        nodes.append(wnode)
        node[1]['index'] = i
        if (source_nodes.count(node[0])):
            sources.append(i)
        i += 1

    # iterate over edges
    i = 0
    for edge in graph.edges_iter(data=True):
        prod  = graph.node[edge[0]]['index']
        con   = graph.node[edge[1]]['index']
        wlink = mod.WorkflowLink(prod,
                                 con,
                                 edge[2]['start_proc'],
                                 edge[2]['nprocs'],
                                 edge[2]['func'],
                                 edge[2]['path'],
                                 edge[2]['prod_dflow_redist'],
                                 edge[2]['dflow_con_redist'])

        # add link to producer and consumer nodes
        nodes[prod].add_out_link(i)
        nodes[con].add_in_link(i)
        i += 1

        links.append(wlink)

    # construct workflow from nodes and links
    wflow = mod.Workflow(nodes, links);

    # run the workflow
    mod.run(wflow, sources)
