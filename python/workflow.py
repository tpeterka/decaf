# converts an nx graph into a workflow data structure and runs the workflow

import imp
import sys
import os
import exceptions
import getopt


class Topology:
  """ Object holder for informations about the plateform to use """

  def __init__(self, name, nProc, hostfilename = "", offsetRank = 0, procPerNode = 0, offsetProcPerNode = 0):

      self.name = name                  # Name of the topology
      self.nProc = nProc                # Total number of MPI ranks in the topology
      self.hostfilename = hostfilename  # File containing the list of hosts. Must match  totalProc
      self.hostlist = []                # List of hosts.
      self.offsetRank = offsetRank      # Rank of the first rank in this topology
      self.nNodes = 0                   # Number of nodes
      self.nodes = []                   # List of nodes
      self.procPerNode = procPerNode    # Number of MPI ranks per node. If not given, deduced from the host list
      self.offsetProcPerNode = offsetProcPerNode    # Offset of the proc ID to start per node with process pinning

      # Minimum level of information required
      if nProc <= 0:
        raise ValueError("Invalid nProc. Trying to initialize a topology with no processes. nProc should be greated than 0.")


      if hostfilename != "":
        f = open(hostfilename, "r")
        content = f.read()
        content = content.rstrip(' \t\n\r') #Clean the last character, usually a \n
        self.hostlist = content.split('\n')
        self.nodes = list(set(self.hostlist))   # Removing the duplicate hosts
        self.nNodes = len(self.nodes)
      else:
        stringHost = "localhost," * (self.nProc - 1)
        stringHost += "localhost"
        self.hostlist = stringHost.split(',')
        self.nodes = ["localhost"]
        self.nNodes = 1

      if len(self.hostlist) != self.nProc:
          raise ValueError("The number of hosts does not match the number of procs.")

  def isInitialized(self):
      return self.nProc > 0

  def toStr(self):
      content = ""
      content += "name: " + self.name + "\n"
      content += "nProc: " + str(self.nProc) + "\n"
      content += "offsetRank: " + str(self.offsetRank) + "\n"
      content += "host list: " + str(self.hostlist) + "\n"

      return content

  def subTopology(self, name, nProcs, procOffset):

      # Enough rank check
      if nProcs > self.nProc:
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
      if sum(nProcs) > self.nProc:
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


def topologyFromArgs(argv):
    """ Create the global topology object from the arguments """
    try:
      opts, args = getopt.getopt(argv,"hn:",["np=","hostfile="])
    except getopt.GetoptError:
      print 'graph.py -n <totalProc> [--hostfile <hostfile>]'
      sys.exit(2)

    nprocs = 0
    hostfile = ""
    name = "global"

    for opt, arg in opts:
      if opt == '-h':
        print 'graph.py -n <totalProc> [--hostfile <hostfile>]'
        sys.exit()
      elif opt in ("-n", "--np"):
        nprocs =   int(arg)
      elif opt in ("", "--hostfile"):
        hostfile = arg

    if nprocs <= 0:
        print "Error: No procs count given."
        print 'graph.py -n <totalProc> [--hostfile <hostfile>]'
        sys.exit()

    return Topology(name, nprocs, hostfilename = hostfile)

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
def workflowToSh(graph, outputFile, mpirunOpt = ""):

    currentRank = 0
    mpirunCommand = "mpirun "+mpirunOpt+" "
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

