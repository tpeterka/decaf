# converts an nx graph into a workflow data structure and runs the workflow

import imp

def workflowToJson(graph, libPath, outputFile):

    nodes   = []
    links   = []

    f = open(outputFile, "w")
    content = ""
    content +="{\n"
    content +="   \"workflow\" :\n"
    content +="   {\n"
    content +="   \"path\" : \""+libPath+"\",\n"
    content +="   \"nodes\" : [\n"

    i = 0
    for node in graph.nodes_iter(data=True):
        content +="       {\n"
        content +="       \"start_proc\" : "+str(node[1]['start_proc'])+" ,\n"
        content +="       \"nprocs\" : "+str(node[1]['nprocs'])+" ,\n"
        content +="       \"func\" : \""+node[1]['func']+"\"\n"
        content +="       },\n"

        node[1]['index'] = i
        i += 1

    content = content.rstrip(",\n")
    content += "\n"
    content +="   ],\n"
    content +="\"edges\" : [\n"

    for edge in graph.edges_iter(data=True):
        prod  = graph.node[edge[0]]['index']
        con   = graph.node[edge[1]]['index']
        content +="       {\n"
        content +="       \"start_proc\" : "+str(edge[2]['start_proc'])+" ,\n"
        content +="       \"nprocs\" : "+str(edge[2]['nprocs'])+" ,\n"
        content +="       \"func\" : \""+edge[2]['func']+"\" ,\n"
        content +="       \"prod_dflow_redist\" : \""+edge[2]['prod_dflow_redist']+"\" ,\n"
        content +="       \"dflow_con_redist\" : \""+edge[2]['dflow_con_redist']+"\" ,\n"
        content +="       \"source\" : "+str(prod)+" ,\n"
        content +="       \"target\" : "+str(con)+" \n"
        content +="       },\n"

    content = content.rstrip(",\n")
    content += "\n"
    content +="   ]\n"
    content +="   }\n"
    content +="}"

    f.write(content)
    f.close()




def workflow(graph,
             run_path):

    nodes   = []
    links   = []

    # load the module
    # hard-coded name 'pymod' must match name of module in PYBIND11_PLUGIN in decaf.hpp
    mod = imp.load_dynamic('pymod', run_path)

    # iterate over nodes
    i = 0
    for node in graph.nodes_iter(data=True):
        wnode = mod.WorkflowNode(node[1]['start_proc'],
                                 node[1]['nprocs'],
                                 node[1]['func'])
        nodes.append(wnode)
        node[1]['index'] = i
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
    mod.run(wflow)

