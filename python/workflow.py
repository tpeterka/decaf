# converts an nx graph into a workflow data structure and runs the workflow

import imp
import sys
import os
import exceptions
import getopt
import argparse

class Topology:
  """ Object holder for informations about the plateform to use """

  def __init__(self, name, nProcs, hostfilename = "", offsetRank = 0, procPerNode = 0, offsetProcPerNode = 0):

      self.name = name                  # Name of the topology
      self.nProcs = nProcs                # Total number of MPI ranks in the topology
      self.hostfilename = hostfilename  # File containing the list of hosts. Must match  totalProc
      self.hostlist = []                # List of hosts.
      self.offsetRank = offsetRank      # Rank of the first rank in this topology
      self.nNodes = 0                   # Number of nodes
      self.nodes = []                   # List of nodes
      self.procPerNode = procPerNode    # Number of MPI ranks per node. If not given, deduced from the host list
      self.offsetProcPerNode = offsetProcPerNode    # Offset of the proc ID to start per node with process pinning

      # Minimum level of information required
      if nProcs <= 0:
        raise ValueError("Invalid nProc. Trying to initialize a topology with no processes. nProc should be greated than 0.")


      if hostfilename != "":
        f = open(hostfilename, "r")
        content = f.read()
        content = content.rstrip(' \t\n\r') #Clean the last character, usually a \n
        self.hostlist = content.split('\n')
        self.nodes = list(set(self.hostlist))   # Removing the duplicate hosts
        self.nNodes = len(self.nodes)
      else:
        stringHost = "localhost," * (self.nProcs - 1)
        stringHost += "localhost"
        self.hostlist = stringHost.split(',')
        self.nodes = ["localhost"]
        self.nNodes = 1

      if len(self.hostlist) != self.nProcs:
          raise ValueError("The number of hosts does not match the number of procs.")

  def isInitialized(self):
      return self.nProcs > 0

  def toStr(self):
      content = ""
      content += "name: " + self.name + "\n"
      content += "nProcs: " + str(self.nProcs) + "\n"
      content += "offsetRank: " + str(self.offsetRank) + "\n"
      content += "host list: " + str(self.hostlist) + "\n"

      return content

  def subTopology(self, name, nProcs, procOffset):

      # Enough rank check
      if nProcs > self.nProcs:
        raise ValueError("Not enough rank available")

      subTopo = Topology(name, nProcs, offsetRank = procOffset)

      # Adjusting the hostlist
      subTopo.hostlist = self.hostlist[procOffset:procOffset+nProcs]
      subTopo.nodes = list(set(subTopo.hostlist))
      subTopo.nNodes = len(subTopo.nodes)

      return subTopo

  def splitTopology(self, names, nProcs):

      # Same size list check
      if len(nProcs) != len (names):
        raise ValueError("The size of the list of procs and names don't match.")

      # Enough rank check
      if sum(nProcs) > self.nProcs:
        raise ValueError("Not enough rank available")

      offset = 0
      splits = []

      for i in range(0, len(names)):
        subTopo = Topology(names[i], nProcs[i], offsetRank = offset)

        subTopo.hostlist = self.hostlist[offset:offset+nProcs[i]]
        subTopo.nodes = list(set(subTopo.hostlist))
        subTopo.nNodes = len(subTopo.nodes)
        offset += nProcs[i]

        splits.append(subTopo)

      return splits

def initParserForTopology(parser):
    """ Add the necessary arguments for initialize a topology.
        The parser might be completed by the user in other functions
    """

    parser.add_argument(
        "-n", "--np",
        help = "Total number of MPI ranks used in the workflow",
        type=int,
        default=1,
        required=True,
        dest="nprocs"
        )
    parser.add_argument(
        "--hostfile",
        help = "List of host for each MPI rank. Must match the number of MPI ranks.",
        default="",
        dest="hostfile"
        )

def topologyFromArgs(args):
    return Topology("global", args.nprocs, hostfilename = args.hostfile)

def processTopology(graph):
    """ Check all nodes and edge if a topology is present.
        If yes, fill the fields start_proc and nprocs """

    for node in graph.nodes_iter(data=True):
        if 'topology' in node[1]:
            topo = node[1]['topology']
            node[1]['start_proc'] = topo.offsetRank
            node[1]['nprocs'] = topo.nProcs

    for edge in graph.edges_iter(data=True):
        if 'topology' in edge[2]:
            topo = edge[2]['topology']
            edge[2]['start_proc'] = topo.offsetRank
            edge[2]['nprocs'] = topo.nProcs


def workflowToJson(graph, libPath, outputFile):

    print "Generating graph description file "+outputFile

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
        content +="       \"target\" : "+str(con)+" ,\n"
        if 'stream' in edge[2]:
            content +="       \"stream\" : \""+edge[2]['stream']+"\", \n"
            content +="       \"storage_types\" : "+str(edge[2]['storage_types']).replace("\'","\"")+", \n"
            content +="       \"max_storage_sizes\" : "+str(edge[2]['max_storage_sizes'])+" \n"
        else:
            content +="       \"stream\" : \"none\" \n"
        content +="       },\n"

    content = content.rstrip(",\n")
    content += "\n"
    content +="   ]\n"
    content +="   }\n"
    content +="}"

    f.write(content)
    f.close()

# Looking for a node/edge starting at a particular rank
def getNodeWithRank(rank, graph):

    for node in graph.nodes_iter(data=True):
        if node[1]['start_proc'] == rank:
            return ('node', node)

    for edge in graph.edges_iter(data=True):
        if edge[2]['start_proc'] == rank:
            return ('edge', edge)

    return ('notfound', graph.nodes_iter())


# Build the mpirun command in MPMD mode
# Parse the graph to sequence the executables with their
# associated MPI ranks and arguments
def workflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):

    print "Generating bash command script "+outputFile

    currentRank = 0
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()

    # Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print 'ERROR: Unable to find an executable for the rank ' + str(rank)
        exit()

      if type == 'node':

        if exe[1]['nprocs'] == 0:
          print 'ERROR: a node can not have 0 MPI rank.'
          exit()

        mpirunCommand += "-np "+str(exe[1]['nprocs'])
        mpirunCommand += " "+str(exe[1]['cmdline'])+" : "
        currentRank += exe[1]['nprocs']

      if type == 'edge':

        if exe[2]['nprocs'] == 0:
          continue #The link is disable

        mpirunCommand += "-np "+str(exe[2]['nprocs'])
        mpirunCommand += " "+str(exe[2]['cmdline'])+" : "
        currentRank += exe[2]['nprocs']

    # Writting the final file
    f = open(outputFile, "w")
    content = ""
    content +="#! /bin/bash\n\n"
    content +=mpirunCommand
    content = content.rstrip(": ")

    f.write(content)
    f.close()
    os.system("chmod a+rx "+outputFile)

# Process the graph and generate the necessary files

def processGraph(graph, name, libPath, mpirunPath = "", mpirunOpt = ""):
    processTopology(graph)
    workflowToJson(graph, libPath, name+".json")
    workflowToSh(graph, name+".sh", mpirunOpt = mpirunOpt, mpirunPath = mpirunPath)
