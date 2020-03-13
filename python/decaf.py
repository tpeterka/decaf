# requires python3

# converts an nx graph into a workflow data structure and runs the workflow

import imp
import sys
import os
#import exceptions # execeptions module not available in python3 (built in instead)
import getopt
import argparse
import json
import networkx as nx
from collections import OrderedDict

from collections import defaultdict

# General graph hosting the entire workflow structure.
# Used to perform graph analysis and generate the json file
workflow_graph = nx.DiGraph()

# Set to True if the user performed an operation which require to
# rank again the topologies once all the topologies are declared
# Ranking the topogies is required when removing nodes from a topology
# or when threading is used
requireTopologyRanking = False


class Topology:
  """ Object holder for informations about the plateform to use """

  def __init__(self, name, nProcs = 0, hostfilename = "", customArgs = "", offsetRank = 0, procPerNode = 0, offsetProcPerNode = 0):

      self.name = name                  # Name of the topology
      self.nProcs = nProcs              # Total number of MPI ranks in the topology
      self.hostfilename = hostfilename  # File containing the list of hosts. Must match  totalProc
      self.customArgs = customArgs      # Optional custom task arguments
      self.hostlist = []                # Complete list of hosts.
      self.offsetRank = offsetRank      # Rank of the first rank in this topology
      self.nNodes = 0                   # Number of nodes
      self.nodes = []                   # List of nodes
      self.procPerNode = procPerNode    # Number of MPI ranks per node. If not given, deduced from the host list
      self.offsetProcPerNode = offsetProcPerNode    # Offset of the proc ID to start per node with process pinning
      self.procs = []                   # List of cores per node
      self.threadPerProc = 1            # Number of threads per MPI rank
      self.useOpenMP = False            # Use OpenMP for this topology, look at the procPerNode to know how many threads per proc

      
      if hostfilename != "":
        f = open(hostfilename, "r")
        content = f.read()
        content = content.rstrip(' \t\n\r') #Clean the last character, usually a \n
        self.hostlist = content.split('\n')
        self.nodes = list(OrderedDict.fromkeys(self.hostlist))   # Removing the duplicate hosts while preserving the order
        self.nNodes = len(self.nodes)
        self.procPerNode = len(self.hostlist) // self.nNodes
        self.nProcs = len(self.hostlist)
        for i in range(0, self.procPerNode):
          self.procs.append(i)

      else:
        self.hostlist = ["localhost"] * self.nProcs # should be the same as the 3 above lines
        self.nodes = ["localhost"]
        for i in range(0, len(self.hostlist)):
          self.procs.append(i)
        self.procPerNode = len(self.procs)
        self.nNodes = 1

  # End of constructor

  def isInitialized(self):
      return self.nProcs > 0


  def toStr(self):
      content = ""
      content += "name: " + self.name + "\n"
      content += "Number of MPI ranks: " + str(self.nProcs) + "\n"
      content += "Threads per MPI rank: " + str(self.threadPerProc) + "\n"
      content += "offsetRank: " + str(self.offsetRank) + "\n"
      content += "host list: " + str(self.hostlist) + "\n"
      content += "core list per node: " + str(self.procs) + "\n"

      return content

  def subTopology(self, name, nProcs, procOffset):

      # Enough rank check
      if procOffset + nProcs > self.nProcs:
        raise ValueError("Not enough rank available. Asked %s, given %s." % (procOffset + nProcs, self.nProcs))

      subTopo = Topology(name, nProcs, offsetRank = procOffset)

      # Adjusting the hostlist
      subTopo.hostlist = self.hostlist[procOffset:procOffset+nProcs]
      subTopo.nodes = list(set(subTopo.hostlist))
      subTopo.nNodes = len(subTopo.nodes)

      #Giving the custom args to subtopos
      subTopo.customArgs = self.customArgs
      return subTopo

  def splitTopology(self, names, nProcs):

      # Same size list check
      if len(nProcs) != len (names):
        raise ValueError("The size of the list of procs and names don't match.")

      # Enough rank check
      if sum(nProcs) > self.nProcs:
        raise ValueError("Not enough rank available. Asked %s, given %s." % (sum(nProcs), self.nProcs))

      offset = 0
      splits = []

      for i in range(0, len(names)):
        subTopo = Topology(names[i], nProcs[i], offsetRank = offset)

        subTopo.hostlist = self.hostlist[offset:offset+nProcs[i]]
        subTopo.nodes = list(set(subTopo.hostlist))
        subTopo.nNodes = len(subTopo.nodes)
        offset += nProcs[i]

        #Giving the custom args to subtopos
        subTopo.customArgs = self.customArgs

        splits.append(subTopo)

      return splits

  def splitTopologyByDict(self, info):

      sum = 0

      for item in info:
          sum = sum + item['nprocs']

      # Enough rank check
      if sum > self.nProcs:
        raise ValueError("Not enough rank available. Asked %s, given %s." % (sum, self.nProcs))

      offset = 0
      splits = {}

      for item in info:
        subTopo = Topology(item['name'], item['nprocs'], offsetRank = offset)

        subTopo.hostlist = self.hostlist[offset:offset+item['nprocs']]
        subTopo.nodes = list(set(subTopo.hostlist))
        subTopo.nNodes = len(subTopo.nodes)
        offset += item['nprocs']

        splits[item['name']] = subTopo

      return splits

  def selectNodeByName(self, name, hosts):

    subNodes = []
    subHostList = []

    for host in hosts:
      #Looking for the hostname name
      if self.nodes.count(host) == 0:
        raise ValueError("Error: Requesting a node not available in the current Topology (" + host + ")")
      subNodes.append(host)
      for j in range(0, self.procPerNode):
        subHostList.append(host)

    # Creating an empty topology
    subTopo = Topology(name)
    subTopo.nodes = subNodes
    subTopo.hostlist = subHostList
    subTopo.nNodes = len(subTopo.nodes)
    subTopo.procs = list(self.procs)
    subTopo.threadPerProc = self.threadPerProc
    subTopo.useOpenMP = self.useOpenMP
    subTopo.procPerNode = len(subTopo.procs) / subTopo.threadPerProc
    subTopo.nProcs = subTopo.nNodes * subTopo.procPerNode

    global requireTopologyRanking
    requireTopologyRanking = True

    return subTopo

  def selectNodeByIndex(self, name, startIndex, nbNodes):
    subNodes = list(self.nodes[startIndex:startIndex+nbNodes])
    subHostList = list(self.hostlist[startIndex*self.procPerNode:(startIndex+nbNodes)*self.procPerNode])

    # Creating an empty topology
    subTopo = Topology(name)
    subTopo.nodes = subNodes
    subTopo.hostlist = subHostList
    subTopo.nNodes = len(subTopo.nodes)
    subTopo.procs = list(self.procs)
    subTopo.threadPerProc = self.threadPerProc
    subTopo.useOpenMP = self.useOpenMP
    subTopo.procPerNode = len(subTopo.procs) / subTopo.threadPerProc
    subTopo.nProcs = subTopo.nNodes * subTopo.procPerNode

    global requireTopologyRanking
    requireTopologyRanking = True

    return subTopo

  def removeNodeByName(self, name, hosts):

    subNodes = list(self.nodes)
    subHostList = list(self.hostlist)

    for host in hosts:
      #Looking for the hostname name
      while subNodes.count(host) > 0:
        subNodes.remove(host)
      while subHostList.count(host) > 0:
        subHostList.remove(host)

    # Creating an empty topology
    subTopo = Topology(name)
    subTopo.nodes = subNodes
    subTopo.hostlist = subHostList
    subTopo.nNodes = len(subTopo.nodes)
    subTopo.procs = list(self.procs)
    subTopo.threadPerProc = self.threadPerProc
    subTopo.useOpenMP = self.useOpenMP
    subTopo.procPerNode = len(subTopo.procs) / subTopo.threadPerProc
    subTopo.nProcs = subTopo.nNodes * subTopo.procPerNode

    global requireTopologyRanking
    requireTopologyRanking = True

    return subTopo

  def removeNodeByIndex(self, name, startIndex, nbNodes):
    subNodes = list(self.nodes)
    subHostList = list(self.hostlist)

    del subNodes[startIndex:startIndex+nbNodes]
    del subHostList[startIndex*self.procPerNode:(startIndex+nbNodes)*self.procPerNode]

    # Creating an empty topology
    subTopo = Topology(name)
    subTopo.nodes = subNodes
    subTopo.hostlist = subHostList
    subTopo.nNodes = len(subTopo.nodes)
    subTopo.procs = list(self.procs)
    subTopo.threadPerProc = self.threadPerProc
    subTopo.useOpenMP = self.useOpenMP
    subTopo.procPerNode = len(subTopo.procs) / subTopo.threadPerProc
    subTopo.nProcs = subTopo.nNodes * subTopo.procPerNode

    global requireTopologyRanking
    requireTopologyRanking = True

    return subTopo

  def selectCores(self, name, cores):
    subNodes = list(self.nodes)

    subHostList = []
    for host in subNodes:
      for i in range(0,len(cores)):
        subHostList.append(host)

    # Checking that all the requested cores are available and not duplicated
    for core in cores:
      if cores.count(core) > 1:
        raise ValueError("ERROR: requesting multiple time the same core within the same topology.")
      if self.procs.count(core) == 0:
        raise ValueError("ERROR: the request core is not available in the current topology.")

    # Checking that the number of threads per core still divide the number of cores
    if len(cores) % self.threadPerProc != 0:
      raise ValueError("ERROR: the number of threads does not divide anymore the number of core for the topology.")

    # Now we know that the cores list is valid
    # Creating an empty topology
    subTopo = Topology(name)
    subTopo.nodes = subNodes
    subTopo.hostlist = subHostList
    subTopo.nNodes = len(subTopo.nodes)
    subTopo.procs = cores
    subTopo.threadPerProc = self.threadPerProc
    subTopo.useOpenMP = self.useOpenMP
    subTopo.procPerNode = len(subTopo.procs) / subTopo.threadPerProc
    subTopo.nProcs = subTopo.nNodes * subTopo.procPerNode

    global requireTopologyRanking
    requireTopologyRanking = True

    return subTopo

  def setThreadsPerRank(self, threadsPerRank):
    if len(self.procs) % threadsPerRank > 0:
      raise ValueError("Error: the requested number of threads per rank.")

    self.threadPerProc = threadsPerRank
    if threadsPerRank > 1:
      self.useOpenMP = True
    else:
      self.useOpenMP = False
    self.procPerNode = len(self.procs) / self.threadPerProc
    self.nProcs = self.nNodes * self.procPerNode

    global requireTopologyRanking
    requireTopologyRanking = True

# End of class Topology

def rankTopologies(topologies):

    currentRank = 0
    for topo in topologies:
      topo.offsetRank = currentRank
      currentRank += topo.nProcs

def initParserForTopology(parser):
    """ Add the necessary arguments to initialize a topology.
        The parser might be completed by the user in other functions
    """
    parser.add_argument(
        "-n", "--np",
        help = "Total number of MPI ranks used in the workflow",
        type=int,
        default=0,
        dest="nprocs"
        )
    parser.add_argument(
        "--hostfile",
        help = "List of host for each MPI rank. Must match the number of MPI ranks.",
        default="",
        dest="hostfile"
        )
    parser.add_argument(
        "-args", "--args",
        help = "Optional custom task arguments",
        type=str,
        default="",
        dest="custom_args"
        )    

def topologyFromArgs(args):
    return Topology("root", args.nprocs, hostfilename = args.hostfile, customArgs = args.custom_args)


# Contract functions
class Filter_level:
  NONE, PYTHON, PY_AND_SOURCE, EVERYWHERE = range(4)
  dictReverse = {
    NONE : "NONE",
    PYTHON : "PYTHON",
    PY_AND_SOURCE : "PY_AND_SOURCE",
    EVERYWHERE : "EVERYWHERE"
  }

class Contract:

  def __init__(self, contract = {}):
    self.contract = contract

  def addEntry(self, name, typename, periodicity = 1):
      self.contract[name] = [typename, periodicity]

  def removeEntry(self, name):
      del self.contract[name]

  def getEntry(self, name):
      return contract[name]

  def printContract(self):
      print(self.contract)


""" Object for contract associated to a link """
class ContractLink:
  def __init__(self, bAny = True, jsonfilename = ""):
    self.bAny = bAny #If bAny set to True, the fields of contract are required but anything else will be forwarded as well
    self.inputs = {}
    self.outputs = {}
    if(jsonfilename != ""):
      with open(jsonfilename, "r") as f:
            content = f.read()
      contract = json.loads(content)
      if "inputs" in contract:
        for key, val in contract["inputs"].items():
          if len(val) == 1:
            val.append(1)
          self.inputs[key] = val
      if "outputs" in contract:
        for key, val in contract["outputs"].items():
          if len(val) == 1:
            val.append(1)
          self.outputs[key] = val
  # End constructor

  def addInput(self, key, typename, period=1):
    self.inputs[key] = [typename, period]

  def addInputFromDict(self, dict):
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1) # period=1 by default
      self.inputs[key] = val

  def addOutput(self, key, typename, period=1):
    self.outputs[key] = [typename, period]

  def addOutputFromDict(self, dict):
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1) # period=1 by default
      self.outputs[key] = val
# End of class ContractLink

class Port:

  def __init__(self, owner, name, direction, contract = None, tokens = 0):
    self.owner = owner          # A Node
    self.name = name            # Name of the port
    self.direction = direction  # 'in' or 'out'
    self.contract = contract    # Contract associated to the port
    self.edges = []             # List of edges connected to this port
    self.tokens = 0             # Number of empty messages to receive on an input port
    if self.direction == 'in':
      if tokens < 0:
          raise ValueError("ERROR: the number of tokens for the port %s.%s should be 0 or higher." % (self.owner.name, self.name))
      self.tokens = tokens

  def addEdge(self, edge):
    if len(self.edges) > 1 and self.direction == 'in':
      raise ValueError("ERROR: cannot connect an input port to several edges\n")

    self.edges.append(edge)

  def setContract(self, contract):
    self.contract = contract

  def getContract(self):
    return self.contract

  def setTokens(self, tokens):
    if self.direction == 'in':
      if tokens < 0:
        raise ValueError("ERROR: the number of tokens for the port %s.%s should be 0 or higher." % (self.owner.name, self.name))
      self.tokens = tokens      # Number of empty messages to receive on an input port

""" Object holding information about a Node """
class Node:
  def __init__(self, name, func, cmdline, start_proc = 0, nprocs = 0, topology = None):
    self.name = name
    self.func = func
    self.cmdline = cmdline
    self.topology = topology

    if self.topology != None:
      self.start_proc = self.topology.offsetRank
      self.nprocs = self.topology.nProcs
    else:
      self.nprocs = nprocs
      self.start_proc = start_proc

    self.inports = {}
    self.outports = {}

    # Adding the node into the workflow
    global workflow_graph
    workflow_graph.add_node(self.name, node=self)

  def addInputPort(self, portname):
    if portname in self.inports:
      raise ValueError("ERROR: portname %s already exist.\n" % (portname))

    self.inports[portname] = Port(self, portname, 'in')
    return self.inports[portname]

  def getInputPort(self, portname):
    assert portname in self.inports

    return self.inports[portname]
#      def addInputPort(self, name, contract=0):
#        if contract == 0:
#          self.inports[name] = {}
#        else:
#          self.inports[name] = contract.inputs
#
#
#      def addInput(self, portname, key, typename, period=1):
#        if not portname in self.inports:
#          self.inports[portname] = {key : [typename, period]}
#        else:
#          self.inports[portname][key] = [typename, period]
#
#      def addInputFromDict(self, portname, dict):
#        if not portname in self.inports:
#          self.inports[portname] = {}
#        for key, val in dict.items():
#          if len(val) == 1:
#            val.append(1)
#          self.inports[portname][key] = val


  def addOutputPort(self, portname):
    if portname in self.outports:
      raise ValueError("ERROR: portname %s already exist.\n" % (portname))

    self.outports[portname] = Port(self, portname, 'out')
    return self.outports[portname]

  def getOutputPort(self, portname):
    assert portname in self.outports

    return self.outports[portname]

#  def addOutputPort(self, name, contract=0):
#    if contract == 0:
#      self.outports[name] = {}
#    else:
#      self.outports[name] = contract.outputs


#  def addOutput(self, portname, key, typename, period=1):
#    if not portname in self.outports:
#      self.outports[portname] = {key : [typename, period]}
#    else:
#      self.outports[portname][key] = [typename, period]
#
#  def addOutputFromDict(self, portname, dict):
#    if not portname in self.outports:
#      self.outports[portname] = {}
#    for key, val in dict.items():
#      if len(val) == 1:
#        val.append(1)
#      self.outports[portname][key] = val
# End of class Node

""" To create a Node using a Topology """
def nodeFromTopo(name, func, cmdline, topology):
  n = Node(name, topology.offsetRank, topology.nProcs, func, cmdline)
  n.topology=topology
  return n


""" Object holding information about an Edge """
class Edge:
  def __init__(self, outPort, inPort, prod_dflow_redist, start_proc=0, nprocs=0, topology=None, func='', path='', dflow_con_redist='', cmdline='', contract=None):

    self.outPort = outPort  # Outport  = Producer
    self.inPort = inPort    # Inport = Consumer

    self.prod_dflow_redist = prod_dflow_redist
    self.contract = contract
    self.src = outPort.owner.name   # Name of the node having the output port
    self.dest = inPort.owner.name   # Name of the node having the input port

    self.outPort.addEdge(self) # Linking the edge to the port
    self.inPort.addEdge(self)

    self.topology = topology
    self.manala = False

    if self.topology != None:
      self.start_proc = self.topology.offsetRank
      self.nprocs = self.topology.nProcs
    else:
      self.nprocs = nprocs
      self.start_proc = start_proc


    if self.nprocs != 0 and cmdline == '':
      raise ValueError("ERROR: Missing links arguments for the Edge \"%s->%s\"." % (src, dest))

    if self.nprocs != 0:
      self.func = func
      self.path = path
      self.dflow_con_redist = dflow_con_redist
      self.cmdline = cmdline

    global workflow_graph
    workflow_graph.add_edge(self.src, self.dest, edge=self)

    #if ('.' in srcString) and ('.' in destString): #In case we have ports
    #  self.srcPort = srcSplit[1]
    #  self.destPort = destSplit[1]
    #else:
    #  self.srcPort = ''
    #  self.destPort = ''

    #self.inputs = {}
    #self.outputs = {}

#  def setAny(self, bAny): # If bAny is set to true, the runtime considers there is a ContractLink, even if inputs and outputs are empty
#    if self.nprocs == 0:
#      print "WARNING: There is no proc in the Edge \"%s->%s\", setting the boolean bAny failed, continuing" % (self.src, self.dest)
#    else:
#      self.bAny = bAny

  # Add streaming information to a link
  def updateLinkStream(self, stream, frame_policy, storage_collection_policy, storage_types, max_storage_sizes, prod_output_freq):
    self.manala = True
    self.stream = stream
    self.frame_policy = frame_policy
    self.storage_collection_policy = storage_collection_policy
    self.storage_types = storage_types
    self.max_storage_sizes = max_storage_sizes
    self.prod_output_freq = prod_output_freq

  # Shortcut function to define a sequential streaming dataflow
  def addSeqStream(self, buffers = ["mainmem"], max_buffer_sizes = [10], prod_output_freq = 1):
    self.updateLinkStream('double', 'seq', 'greedy', buffers, max_buffer_sizes, prod_output_freq)

  # Shortcut function to define a stream sending the most recent frame to the consumer
  def addMostRecentStream(self, buffers = ["mainmem"], max_buffer_sizes = [10], prod_output_freq = 1):
    self.updateLinkStream('single', 'recent', 'lru', buffers, max_buffer_sizes, prod_output_freq)

  # Shortcut function to define a stream sending if possible at the high frequency and guarantee to send at the low frequency
  def addLowHighStream(self, low_freq, high_freq, buffers = ["mainmem"], max_buffer_sizes = [10]):
    self.updateLinkStream('single', 'lowhigh', 'greedy', buffers, max_buffer_sizes, 1)
    self.low_output_freq = low_freq
    self.high_output_freq = high_freq

  def addDirectSyncStream(self, prod_output_freq = 1):
    self.updateLinkStream('single', 'seq', 'greedy', [], [], prod_output_freq)


  def setContractLink(self, cLink):
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", adding a ContractLink is useless, continuing." %
              (self.src, self.dest))
#    self.inputs = cLink.inputs
#    self.outputs = cLink.outputs
#    self.bAny = cLink.bAny
    else:
      self.contract = cLink

#  def addInputFromDict(self, dict):
#    if self.nprocs == 0:
#      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Inputs is useless, continuing." % (self.src, self.dest)
#    for key, val in dict.items():
#      if len(val) == 1:
#        val.append(1)
#      self.inputs[key] = val

#  def addInput(self, key, typename, period=1):
#    if self.nprocs == 0:
#      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Inputs is useless, continuing." % (self.src, self.dest)
#    self.inputs[key] = [typename, period]


#  def addOutputFromDict(self, dict):
#    if self.nprocs == 0:
#      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Outputs is useless, continuing." % (self.src, self.dest)
#    for key, val in dict.items():
#      if len(val) == 1:
#        val.append(1)
#      self.outputs[key] = val

#  def addOutput(self, key, typename, period=1):
#    if self.nprocs == 0:
#      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Outputs is useless, continuing." % (self.src, self.dest)
#    self.outputs[key] = [typename, period]
# End of class Edge

""" To create an Edge with a Topology """
def edgeFromTopo(srcString, destString, topology, prod_dflow_redist, func='', path='', dflow_con_redist='', cmdline=''):
    e = Edge(srcString, destString, topology.offsetRank, topology.nProcs, prod_dflow_redist, func, path, dflow_con_redist, cmdline)
    e.topology = topology
    return e


""" To easily add a Node to the nx graph """
def addNode(graph, n):
  graph.add_node(n.name, node=n)

""" To easily add an Edge to the nx graph """
def addEdge(graph, e):
  graph.add_edge(e.src, e.dest, edge=e)





# Checks the contracts between each pair of producer/consumer, taking into account the contracts on links if any
def check_contracts(graph, filter_level):
    if filter_level == Filter_level.NONE:
      return
    # TODO remove all checks of filter_level == NONE in the rest of this function (and function checkWithPorts, checkWithoutPorts also) !!!
    # Well, check if we can remove it...
      
    my_list = [] # To check if an input port is connected to only one output port
    
    for graphEdge in graph.edges(data=True):
        edge = graphEdge[2]["edge"]              # Edge object
        prod = graph.nodes[graphEdge[0]]["node"]  # Node object for prod
        con = graph.nodes[graphEdge[1]]["node"]   # Node object for con

        #if edge.srcPort != '' and edge.destPort != '' : # If the prod/con of this edge have ports
        (keys, keys_link, attachAny) = checkWithPorts(edge, prod, con, my_list, filter_level)
        if keys != 0:
          graphEdge[2]['keys'] = keys
        if keys_link != 0:
          graphEdge[2]['keys_link'] = keys_link
        if attachAny == True:
          graphEdge[2]['bAny'] = edge.bAny
        # Else: there are no ports so no contracts
# End of function check_contracts

# Return values: keys, keys_link, attachAny
# keys is the dict of fields to be exchanged b/w producer and consumer, or producer and link if there is a contract on the link
# keys_link is the dict of fields to be exchanged b/w link and consumer if there is a contract of the link
# attachAny is a boolean to say if the link will forward any field. Only used when the contracts of both ports are empty
def checkWithPorts(edge, prod, con, my_list, filter_level):
  #OutportName = edge.srcPort
  OutportName = edge.outPort.name
  #Outport = prod.outports[OutportName]
  OutportContract = edge.outPort.contract.contract # Table of entries

  #InportName = edge.destPort
  InportName = edge.inPort.name
  #Inport = con.inports[InportName]
  InportContract = edge.inPort.contract.contract # Table of entries

  Link = edge.contract

  # No need to check for multiple connections here, already done at the creation of the edge
  #s = edge.dest+'.'+InportName
  #if s in my_list: # This input port is already connected to an output port
  #  raise ValueError("ERROR: The Input port \"%s\" cannot be connected to more than one Output port, aborting." % (s) )
  #else:
  #  my_list.append(s)

  if edge.nprocs != 0 and hasattr(Link,'bAny'):
    keys = {}
    keys_link = {}
    attachAny = False


    # Match contract Prod with link input

    # Case with no input contract on the link
    if len(Link.inputs) == 0:
      if Link.bAny == False:
        raise ValueError("ERROR: The link of \"%s.%s->%s\" has no inputs and bAny is set to False, aborting." % (edge.src, OutportName, InportName))
      else:
        if len(OutportContract) == 0:
          keys = 0 # No need to perform filtering at runtime
        else:
          intersection_keys = {key:[InportContract[key][0]] for key in InportContract.keys() if (key in OutportContract) and ( (filter_level == Filter_level.NONE) or (InportContract[key][0] == OutportContract[key][0]) ) }
          for key in intersection_keys.keys():
            new_val = InportContract[key][1] * OutportContract[key][1]
            intersection_keys[key].append(new_val)
            #if new_val != Inport[key][1]: # no need to print this warning since the field is just here to be forwarded to the consumer
            #  print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, new_val, con_in[key][1])
          if len(intersection_keys) != 0:
            print("WARNING: the link of \"%s.%s->%s.%s\" has no input contract, only fields needed by the input port of the consumer will be received" % (edge.src, OutportName, edge.dest, InportName))
            keys = intersection_keys
          else: # There is nothing to sent by the producer, would block at runtime
            raise ValueError("ERROR: the link of \"%s.%s->%s.%s\" has no inputs and there is no field in common between the Output and Input ports, aborting." % (edge.src, OutportName, edge.dest, InportName))

    # Case with an input contract on the link
    else: # len(edge.inputs) != 0
      if len(OutportContract) == 0:
        if Link.bAny == True:
          print("WARNING: The link of \"%s.%s->%s.%s\" will accept anything from the Output port." %
                  (edge.src, OutportName, edge.dest, InportName))
          if len(InportContract) == 0:
            attachAny = True
            keys = Link.inputs
          else:
            keys = 0
        else:
          print("WARNING: The Output port of \"%s.%s->%s.%s\" can send anything, only fields needed by the link will be kept, the periodicity may not be correct." %
                  (edge.src, OutportName, edge.dest, InportName))
          keys = Link.inputs
      else : # len(Outport) != 0
        list_fields = {}
        sk = ""
        for key, val in Link.inputs.items():
          if not key in Outport:
            sk += key + ", "
          else:
            if (filter_level == Filter_level.NONE) or (val[0] == OutportContract[key][0]):
              new_val = val[1] * OutportContract[key][1]
              list_fields[key] = [val[0], new_val]
              if new_val != val[1]:
                print("WARNING: the link of \"%s.%s->%s.%s\" will receive %s with periodicity %s instead of %s." %
                        (edge.src, OutportName, edge.dest, InportName, key, new_val, val[1]))
            else:
              raise ValueError("ERROR: the types of \"%s\" does not match for the Output port and the link of \"%s.%s->%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))
        if sk != "":
          raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port of \"%s.%s->%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName, edge.dest, InportName))
        keys = list_fields
        if edge.bAny == True: # We complete with the other keys in Outport if they are in Inport and of the same type
          for key, val in OutportContract.items():
            if (not key in keys) and ( (len(InportContract) == 0) or ( key in InportContract and OutportContract[key][0] == InportContract[key][0] ) ):
              keys[key] = val

    #Match link output with contract Con
    if len(Link.outputs) == 0:
      if Link.bAny == False:
        raise ValueError("ERROR: The link of \"%s.%s->%s.%s\" has no outputs and bAny is set to False, aborting." % (edge.src, OutportName, edge.dest, InportName))
      else:
        if len(InportContract) == 0:
          keys_link = 0 # No need to perform filtering at runtime
        else: # Checks if keys of Inport are a subset of Outport
          list_fields = {}
          sk = ""
          for key, val in InportContract.items():
            if not key in OutportContract : # The key is not sent by the producer
              sk+= key + ", "
            else:
              if (filter_level == Filter_level.NONE) or (val[0] == OutportContract[key][0]) : # The types match, add the field to the list and update the periodicity
                new_val = val[1] * OutportContract[key][1]
                list_fields[key] = [val[0], new_val]
                if new_val != val[1]:
                  print("WARNING: %s will receive %s with periodicity %s instead of %s." %
                          (edge.dest, key, new_val, val[1]))
              else:
                raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

          if sk != "":
            raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
          keys_link = list_fields
          print("WARNING: The link of \"%s.%s->%s.%s\" has no output contract, all fields needed by the Input port of the consumer will be forwarded." %
                  (edge.src, OutportName, edge.dest, InportName))
    else: # len(edge.outputs) != 0
      if len(InportContract) == 0:
        if Link.bAny == False:
          keys_link = Link.outputs
        else: # Need to attach keys of output link and of output Port
          print("WARNING: The link of \"%s.%s->%s.%s\" will forward anything to the Input port." %
                  (edge.src, OutportName, edge.dest, InportName))
          if len(OutportContract) == 0: # If there are no contracts on Nodes but only contract on the Link
            attachAny = True
            keys_link = edge.outputs
          else:
            for key, val in edge.outputs.items():
              keys_link[key] = val
            for key, val in OutportContract.items():
              if not key in keys_link:
                keys_link[key] = val
      else: # len(Inport) != 0
        list_fields = {}
        tmp = {}
        for key, val in InportContract.items():
          if not key in edge.outputs:
            tmp[key] = val
          else:
            if (filter_level == Filter_level.NONE) or (val[0] == Link.outputs[key][0]):
              new_val = val[1] * Link.outputs[key][1]
              list_fields[key] = [val[0], new_val]
              if new_val != val[1]:
                print("WARNING: the Input port of \"%s.%s->%s.%s\" will receive %s with periodicity %s instead of %s." %
                        (edge.src, OutportName, edge.dest, InportName, key, new_val, val[1]))
            else:
              raise ValueError("ERROR: the types of \"%s\" does not match for the link and the Input port of \"%s.%s->%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))
        if edge.bAny == False:
          if len(tmp) == 0:
            keys_link = list_fields
          else:
            sk = ""
            for key in tmp.keys():
              sk += key + ", "
            raise ValueError("ERROR: the fields \"%s\" are not sent by the link of \"%s.%s->%s.%s\", aborting" % (sk.rstrip(", "), edge.src, OutportName, edge.dest, InportName))
        else: #bAny == True, need to complete with the keys sent by the Prod
          if len(OutportContract) == 0: # The Outport can send anything, assume the missing keys are sent
            for key, val in tmp.items():
              list_fields[key] = val
            print("WARNING: The link of \"%s.%s->%s.%s\" will forward anything from the Output port, cannot guarantee the fields needed by the Input port will be present." %
                    (edge.src, OutportName, edge.dest, InportName))
            keys_link = list_fields
          else: # Complete with the keys of the Output port
            sk = ""
            for key, val in tmp.items():
              if not key in OutportContract:
                sk += key + ", "
              else:
                if (filter_level == Filter_level.NONE) or (val[0] == OutportContract[key][0]):
                  new_val = val[1] * OutportContract[key][1]
                  list_fields[key] = [val[0], new_val]
                  if new_val != val[1]:
                    print("WARNING: the Input port of \"%s.%s->%s.%s\" will receive %s with periodicity %s instead of %s." %
                            (edge.src, OutportName, edge.dest, InportName, key, new_val, val[1]))
                else:
                  raise ValueError("ERROR: the types of \"%s\" does not match for the Output port and the Input port of \"%s.%s->%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))
            if sk != "":
              raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port of \"%s.%s->%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName, edge.dest, InportName))
            keys_link = list_fields

    return (keys, keys_link, attachAny)
  # End of if hasattr(edge,'bAny')
  else:
    if len(InportContract) == 0: # The Input port accepts anything, send only keys in the output port contract
      if len(OutportContract) == 0:
        print("WARNING: The port \"%s\" will accept everything sent by \"%s.%s\"." %
                (InportName, edge.src, OutportName))
        return (0,0, False)
      else:
        return (OutportContract, 0, False)

    if len(OutportContract) == 0: # The Output port can send anything, keep only the list of fields needed by the consumer
      print("WARNING: The Output port \"%s.%s\" can send anything, only fields needed by the Input port \"%s\" will be kept, the periodicity may not be correct." %
              (edge.src, OutportName, InportName))
      return (InportContract, 0, False)

    else: # Checks if the list of fields of the consumer is a subset of the list of fields from the producer
      list_fields = {}
      sk = ""
      for key, val in InportContract.items():
        if not key in OutportContract : # The key is not sent by the producer
          sk+= key + ", "
        else:
          if (filter_level == Filter_level.NONE) or (val[0] == OutportContract[key][0]) : # The types match, add the field to the list and update the periodicity
            new_val = val[1] * OutportContract[key][1]
            list_fields[key] = [val[0], new_val]
            if new_val != val[1]:
              print("WARNING: %s will receive %s with periodicity %s instead of %s." %
                      (edge.dest, key, new_val, val[1]))
          else:
            raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

      if sk != "":
        raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
    return (list_fields, 0, False) # This is the dict of fields to be exchanged in this edge; A field is an entry of a dict: name:[type, period]
# end of checkWithPorts



def checkCycles(graph):

  cycle_list = list(nx.simple_cycles(graph))
  if len(cycle_list) > 0:
    print("Decaf detected " + str(len(cycle_list)) + " cycles. Checking for tokens...")

  for cycle in cycle_list:
    # Checking if one of the node in the cycle has at least 1 token
    found_token = False
    for node in cycle:

      # The list given by simple_cycles does not have the attribute
      # We have to find back the correct node
      for val in graph.nodes(data=True):
        if val[0] == node:
          for name,port in val[1]['node'].inports.items():
            if port.tokens > 0:
              found_token = True
    if found_token:
      print("Token detected in the cycle.")
    else:
      raise ValueError("A cycle was detected within the cycle %s, but no token were inserted. Please insert at least one token in one of the cycle's tasks." % (cycle))





def workflowToJson(graph, outputFile, filter_level):
    print("Generating graph description file "+outputFile)

    nodes   = []
    links   = []
    list_id = []

    data = {}
    data["workflow"] = {}
    data["workflow"]["filter_level"] = Filter_level.dictReverse[filter_level]

    data["workflow"]["nodes"] = []
    i = 0
    for val in graph.nodes(data=True):
        node = val[1]["node"]
        data["workflow"]["nodes"].append({"start_proc" : node.start_proc,
                                          "nprocs" : node.nprocs,
                                          "func" : node.func})
        inputPorts = []
        for port in node.inports:
            inputPorts.append(port)

        outputPorts = []
        for port in node.outports:
            outputPorts.append(port)

        data["workflow"]["nodes"][i].update({"inports" : inputPorts,
                                            "outports" : outputPorts})

        val[1]['index'] = i
        i += 1


    data["workflow"]["edges"] = []
    i = 0
    for graphEdge in graph.edges(data=True):
        edge = graphEdge[2]["edge"]
        prod_id  = graph.nodes[graphEdge[0]]['index']
        con_id   = graph.nodes[graphEdge[1]]['index']
        data["workflow"]["edges"].append({"start_proc" : edge.start_proc,
                                          "nprocs" : edge.nprocs,
                                          "source" : prod_id,
                                          "target" : con_id,
                                          "prod_dflow_redist" : edge.prod_dflow_redist,
                                          "name" : graphEdge[0] + "_" + graphEdge[1],
                                          "sourcePort" : edge.outPort.name,
                                          "targetPort" : edge.inPort.name,
                                          "tokens" : edge.inPort.tokens})

        if "transport" in graphEdge[2]:
            data["workflow"]["edges"][i]["transport"] = graphEdge[2]['transport']
        else:
            data["workflow"]["edges"][i]["transport"] = "mpi"

        if edge.nprocs != 0:
          data["workflow"]["edges"][i].update({"func" : edge.func,
                                          "dflow_con_redist" : edge.dflow_con_redist,
                                          "path" : edge.path})
          # Used for ContractLink
          if "keys_link" in graphEdge[2]:
            data["workflow"]["edges"][i]["keys_link"] = graphEdge[2]['keys_link']
            if "bAny" in graphEdge[2]:
              data["workflow"]["edges"][i]["bAny"] = graphEdge[2]['bAny']    

        # Used for Contract
        if "keys" in graphEdge[2]:
          data["workflow"]["edges"][i]["keys"] = graphEdge[2]['keys']
        
        if edge.manala == True:
            data["workflow"]["edges"][i].update({ "stream" : edge.stream,
                                                  "frame_policy" : edge.frame_policy,
                                                  "storage_collection_policy" : edge.storage_collection_policy,
                                                  "storage_types" : edge.storage_types,
                                                  "max_storage_sizes" : edge.max_storage_sizes})
            if hasattr(edge, "prod_output_freq"):
                data["workflow"]["edges"][i].update({"prod_output_freq" : edge.prod_output_freq})
            if hasattr(edge, "low_output_freq"):
                data["workflow"]["edges"][i].update({"low_output_freq" : edge.low_output_freq})
            if hasattr(edge, "high_output_freq"):
                data["workflow"]["edges"][i].update({"high_output_freq" : edge.high_output_freq})
        # If there are ports for this edge
        #if edge.srcPort != '' and edge.destPort != '':
        #  data["workflow"]["edges"][i].update({"sourcePort":edge.srcPort,
        #                                      "targetPort":edge.destPort})


        i += 1


    f = open(outputFile, 'w')
    json.dump(data, f, indent=4)
    f.close()
# End workflowToJson

def checkTopologyRanking(graph):
    if not requireTopologyRanking:
        # We don't need to update the ranking of the topologies,
        # it is already setup and consistent
        print("No need to update the topologies ranking.")
        return
    else:
        print("Updating the topologies ranking.")

    # Collecting all the topologies in the graph
    topologies = []
    for val in graph.nodes(data=True):
        node = val[1]["node"]
        if hasattr(node, 'topology') and node.topology != None:
            topologies.append(node.topology)
        else:
            raise ValueError("ERROR: no topology attribute while updating the ranking.")


    for val in graph.edges(data=True):
        edge = val[2]["edge"]
        if hasattr(edge, 'topology') and edge.topology != None:
            topologies.append(edge.topology)
        elif edge.nprocs > 0:
            raise ValueError("ERROR: no topology attribute while updating the ranking.")

    # Updating the ranking in the topology
    rankTopologies(topologies)

    # Updating the start_proc and nprocs in the Nodes and edges
    # This is for retro compatibility: previous API did not have
    # topologies and their MPI commands were generated from the
    # start_proc and nprocs attributes
    for val in graph.nodes(data=True):
        node = val[1]["node"]
        node.start_proc = node.topology.offsetRank
        node.nprocs = node.topology.nProcs
        #print "Setting the topology for " + node.name + " start: "+str(node.start_proc)


    for val in graph.edges(data=True):
        edge = val[2]["edge"]
        if edge.nprocs > 0:
            edge.start_proc = edge.topology.offsetRank
            edge.nprocs = edge.topology.nProcs

# Looking for a node/edge starting at a particular rank
def getNodeWithRank(rank, graph):

    for val in graph.nodes(data=True):
        node = val[1]["node"]
        if node.start_proc == rank:
            return ('node', node)

    for val in graph.edges(data=True):
        edge = val[2]["edge"]
        if (edge.start_proc == rank) and (edge.nprocs != 0):
            return ('edge', edge)

    #return ('notfound', graph.nodes())
    return ('notfound', 0) # should be the same since second value is not used in this case


def workflowToSh(graph, outputFile, inputFile, mpirunOpt = "", mpirunPath = "", envTarget = "generic"):
    print("Selecting the transport method...")

    transport = ""
    for graphEdge in graph.edges(data=True):
        edge = graphEdge[2]["edge"]
        if "transport" in graphEdge[2]:
            if transport == "":
              transport = graphEdge[2]["transport"]
            elif transport != graphEdge[2]["transport"]:
              raise ValueError("ERROR: Mixing transport communication methods.")
        elif transport == "":
            transport = "mpi"
        elif transport != "mpi":
            raise ValueError("ERROR: Mixing transport communication methods.")

    if graph.number_of_edges() == 0:
        print("Selected transport method: None")
    else:
        print("Selected transport method: "+transport)

    if inputFile == "":
      print("No input file is provided")
      
    if graph.number_of_edges() == 0:
        MPIworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport == "mpi" and inputFile != "":
      SPMDworkflowToSh(graph,outputFile,inputFile,mpirunOpt,mpirunPath)
    elif transport == "mpi" and envTarget == "generic":
      MPIworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport ==  "mpi" and envTarget == "openmpi":
      OpenMPIworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport == "cci":
      MPMDworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport == "file":
      MPMDworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    else:
      raise ValueError("ERROR: Unknow transport method selected %s" % (transport))

# Build the mpirun command in MPMD mode
# Parse the graph to sequence the executables with their
# associated MPI ranks and arguments
def OpenMPIworkflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
    print("Generating bash command script "+outputFile+ " for OpenMPI.")

    currentRank = 0
    hostlist = []
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" --hostfile hostfile_workflow.txt --rankfile rankfile_workflow.txt "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()
    rankfileContent = ""

    #print "Number of executable to find: %s" % (nbExecutables)

    # Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print('ERROR: Unable to find an executable for the rank ' + str(rank))
        exit()

      if type == 'node':
        #print "Processing a node..."
        if exe.nprocs == 0:
          print('ERROR: a node can not have 0 MPI rank.')
          exit()

        mpirunCommand += "-np "+str(exe.nprocs)

        #Checking if a topology is specified.
        if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
          for host in exe.topology.hostlist:
            hostlist.append(host)
        else:
            raise ValueError("ERROR: The target OpenMPI requires to use Topologies. Use Topologies to allocate resources to each task or request the generic target instead. ")

        # Generating the rankfile entries for each rank
        hostfileRank = currentRank
        for host in exe.topology.nodes:
          for index in range(0, exe.topology.procPerNode):
            rankfileContent += "rank %s=%s slot=" % (hostfileRank,host)
            for coreIndex in range(index*exe.topology.threadPerProc, (index+1)*exe.topology.threadPerProc):
              rankfileContent+=str(exe.topology.procs[coreIndex]) +','
            rankfileContent = rankfileContent.rstrip(',') + '\n'
            hostfileRank += 1
        #print "Content after the node " + exe.name
        #print rankfileContent

        mpirunCommand += " "+str(exe.cmdline)+" : "
        currentRank += exe.nprocs

      if type == 'edge':
        print("Processing an edge")
        if exe.nprocs != 0:
          mpirunCommand += "-np "+str(exe.nprocs)

          #Checking if a topology is specified. If no, fill a filehost with localhost
          if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
            for host in exe.topology.hostlist:
              hostlist.append(host)
          else:
              raise ValueError("ERROR: The target OpenMPI requires to use Topologies. Use Topologies to allocate resources to each task or request the generic target instead. ")

          # Generating the rankfile entries for each rank
          hostfileRank = currentRank
          for host in exe.topology.nodes:
            for index in range(0, exe.topology.procPerNode):
              rankfileContent += "rank %s=%s slot="
              for coreIndex in range(index*exe.topology.threadPerProc, (index+1)*exe.topology.threadPerProc):
                rankfileContent+=str(exe.topology.procs[coreIndex])+','
              rankfileContent = rankfileContent.rstrip(',') + '\n'
              hostfileRank += 1

          mpirunCommand += " "+str(exe.cmdline)+" : "
          currentRank += exe.nprocs

    # Writting the final file
    f = open(outputFile, "w")
    content = ""
    content +="#! /bin/bash\n\n"
    content +=mpirunCommand
    content = content.rstrip(" : ") + "\n"

    f.write(content)
    f.close()
    os.system("chmod a+rx "+outputFile)

    #Writing the hostfile
    f = open("hostfile_workflow.txt","w")
    content = ""
    for host in hostlist:
      content += host + "\n"
    content = content.rstrip("\n")
    f.write(content)
    f.close()

    #print "Printing the content of the rankfile..."
    rankfileContent = rankfileContent.rstrip('\n')
    f = open("rankfile_workflow.txt", "w")
    f.write(rankfileContent)
    f.close()
    #print rankfileContent

# Build the mpirun command in MPMD mode
# Parse the graph to sequence the executables with their
# associated MPI ranks and arguments, generate a hostfile and rankfile
# to associate the correct CPU enveloppe to each
def MPIworkflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
    print("Generating bash command script "+outputFile+" for a generic MPI environment.")

    currentRank = 0
    hostlist = []
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" --hostfile hostfile_workflow.txt "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()

    # Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print('ERROR: Unable to find an executable for the rank ' + str(rank))
        exit()

      if type == 'node':

        if exe.nprocs == 0:
          print('ERROR: a node can not have 0 MPI rank.')
          exit()

        mpirunCommand += "-np "+str(exe.nprocs)

        #Checking if a topology is specified. If no, fill a filehost with localhost
        if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
          mpirunCommand += " "+str(exe.cmdline)+" "+str(exe.topology.customArgs)+" : "
          for host in exe.topology.hostlist:
            hostlist.append(host)
        else:
          mpirunCommand += " "+str(exe.cmdline)+" : "
          for j in range(0, exe.nprocs):
            hostlist.append("localhost")
        currentRank += exe.nprocs

      if type == 'edge':

        if exe.nprocs != 0:
          mpirunCommand += "-np "+str(exe.nprocs)

          #Checking if a topology is specified. If no, fill a filehost with localhost
          if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
            mpirunCommand += " "+str(exe.cmdline)+" "+str(exe.topology.customArgs)+" : "
            for host in exe.topology.hostlist:
              hostlist.append(host)
          else:
            mpirunCommand += " "+str(exe.cmdline)+" : "
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")
          currentRank += exe.nprocs

    # Writting the final file
    f = open(outputFile, "w")
    content = ""
    content +="#! /bin/bash\n\n"
    content +=mpirunCommand
    content = content.rstrip(" : ") + "\n"

    f.write(content)
    f.close()
    os.system("chmod a+rx "+outputFile)

    #Writing the hostfile
    f = open("hostfile_workflow.txt","w")
    content = ""
    for host in hostlist:
      content += host + "\n"
    content = content.rstrip("\n")
    f.write(content)
    f.close()

def SPMDworkflowToSh(graph, outputFile, inputFile, mpirunOpt = "", mpirunPath = ""):
    print("Generating bash command script "+outputFile+" for a generic MPI environment.")

    currentRank = 0
    hostlist = []
    mode = "spmd"
    flag = 0
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" --hostfile hostfile_workflow.txt "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()
    
    # Iterating over the graph for finding about the mode (MPMD vs SPMD) 
    for i in range(0, nbExecutables):
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print('ERROR: Unable to find an executable for the rank ' + str(rank))
        exit()

      if type == 'node':
        if flag == 0:
          nameExec = str(exe.cmdline)
          flag = 1
        if str(exe.cmdline) != nameExec:
          mode = "mpmd"
        currentRank += exe.nprocs
          
      if type == 'edge' and exe.nprocs!=0:
        currentRank += exe.nprocs

    currentRank = 0
    
    # Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      (type, exe) = getNodeWithRank(currentRank, graph)
      #if type == 'none':
      #  print('ERROR: Unable to find an executable for the rank ' + str(rank))
      #  exit()

      if type == 'node':

        if exe.nprocs == 0:
          print('ERROR: a node can not have 0 MPI rank.')
          exit()

        if mode == 'mpmd':
          mpirunCommand += "-np "+str(exe.nprocs)

        #Checking if a topology is specified. If no, fill a filehost with localhost
        if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
          for host in exe.topology.hostlist:
            hostlist.append(host)
        else:
            #print "No topology found."
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")
        currentRank += exe.nprocs
        #print('node' + str(currentRank))
        if mode == 'mpmd':             
          mpirunCommand += " "+str(exe.cmdline)+" "+str(inputFile)+" : "


      if type == 'edge':

        if exe.nprocs != 0:
          if mode == 'mpmd':
            mpirunCommand += "-np "+str(exe.nprocs)

          #Checking if a topology is specified. If no, fill a filehost with localhost
          if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
            for host in exe.topology.hostlist:
              hostlist.append(host)
          else:
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")
          if mode == 'mpmd':
            mpirunCommand += " "+str(exe.cmdline)+" "+str(inputFile)+" : "
          currentRank += exe.nprocs

    if mode == 'spmd':
      mpirunCommand += "-np "+str(currentRank)+" "+str(nameExec)+" "+str(inputFile)
    # Writing the final file
    f = open(outputFile, "w")
    content = ""
    content +="#! /bin/bash\n\n"
    content +=mpirunCommand
    content = content.rstrip(" : ") + "\n"

    f.write(content)
    f.close()
    os.system("chmod a+rx "+outputFile)

    #Writing the hostfile
    f = open("hostfile_workflow.txt","w")
    content = ""
    for host in hostlist:
      content += host + "\n"
    content = content.rstrip("\n")
    f.write(content)
    f.close()

"""Build the various mpirun commands for each executable
   The function generate the environment variables necesaries for Decaf
   to recreate the ranking of the workflow
"""
def MPMDworkflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
    print("Generating bash command script for MPMD "+outputFile)

    currentRank = 0
    totalRank = 0
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()

    commands = []

    # Computing the total number of ranks with the workflow
    for val in graph.nodes(data=True):
        node = val[1]["node"]
        totalRank+=node.nprocs


    for graphEdge in graph.edges(data=True):
        edge = graphEdge[2]["edge"]
        totalRank+=edge.nprocs

    #print "Total number of ranks in the workflow: "+str(totalRank)

    #Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      hostlist = []
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print('ERROR: Unable to find an executable for the rank ' + str(rank))
        exit()

      hostfilename = "hostfile_task"+str(i)
      if type == 'node':

        if exe.nprocs == 0:
          print('ERROR: a node can not have 0 MPI rank.')
          exit()

        command = mpirunCommand+"-np "+str(exe.nprocs)+" --hostfile "+hostfilename
        command+=" -x DECAF_WORKFLOW_SIZE="+str(totalRank)+" -x DECAF_WORKFLOW_RANK="+str(currentRank)
        command += " "+str(exe.cmdline)+" &"
        currentRank += exe.nprocs

        #Checking if a topology is specified. If no, fill a filehost with localhost
        if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
          for host in exe.topology.hostlist:
            hostlist.append(host)
        else:
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")
        commands.append(command)

        f = open(hostfilename,"w")
        content = ""
        for host in hostlist:
          content += host + "\n"
        content = content.rstrip("\n")
        f.write(content)
        f.close()

      if type == 'edge':

        if exe.nprocs != 0:
          command = mpirunCommand+"-np "+str(exe.nprocs)+" --hostfile "+hostfilename
          command+=" -x DECAF_WORKFLOW_SIZE="+str(totalRank)+" -x DECAF_WORKFLOW_RANK="+str(currentRank)
          command += " "+str(exe.cmdline)+" &"
          currentRank += exe.nprocs
          commands.append(command)

          #Checking if a topology is specified. If no, fill a filehost with localhost
          if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
            for host in exe.topology.hostlist:
              hostlist.append(host)
          else:
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")

          f = open(hostfilename,"w")
          content = ""
          for host in hostlist:
            content += host + "\n"
          content = content.rstrip("\n")
          f.write(content)
          f.close()

    # Writting the final file
    f = open(outputFile, "w")
    content = ""
    content +="#! /bin/bash\n\n"
    for command in commands:
      content+=command+"\n"

    f.write(content)
    f.close()
    os.system("chmod a+rx "+outputFile)

""" DEPRECATED: Used for the retrocompatibility when not using Node and Edge objects in the graph
    For each node and edge of the graph, retrieves its attributes and replace them by the corresponding object
"""
def createObjects(graph):
  for node in graph.nodes(data=True):
    if not 'node' in node[1]:
      if 'topology' in node[1]:
        my_node = nodeFromTopo(node[0], node[1]['func'], node[1]['cmdline'], node[1]['topology'])
        del node[1]['topology']
      else:
        my_node = Node(node[0], node[1]['start_proc'], node[1]['nprocs'], node[1]['func'], node[1]['cmdline'])
        del node[1]['start_proc']
        del node[1]['nprocs']
      if 'tokens' in node[1]:
          my_node.setTokens(node[1]['tokens'])
          del node[1]['tokens']

      node[1]['node'] = my_node # Insert the object and clean the other attributes
      del node[1]['func']
      del node[1]['cmdline']
      
  for edge in graph.edges(data=True):
    if not 'edge' in edge[2]:
      if 'topology' in edge[2]:
        if edge[2]['topology'].nProcs != 0:
          my_edge = edgeFromTopo(edge[0], edge[1], edge[2]['topology'], edge[2]['prod_dflow_redist'], edge[2]['func'], edge[2]['path'],
                                edge[2]['dflow_con_redist'], edge[2]['cmdline'])
          del edge[2]['func']
          del edge[2]['path']
          del edge[2]['dflow_con_redist']
          del edge[2]['cmdline']
        else: # nprocs == 0
          my_edge = edgeFromTopo(edge[0], edge[1], edge[2]['topology'], edge[2]['prod_dflow_redist'])

        del edge[2]['prod_dflow_redist']
        del edge[2]['topology']
      else: # no topology
        if edge[2]['nprocs'] != 0:
          my_edge = Edge(edge[0], edge[1], edge[2]['start_proc'], edge[2]['nprocs'], edge[2]['prod_dflow_redist'], edge[2]['func'],
                        edge[2]['path'], edge[2]['dflow_con_redist'], edge[2]['cmdline'])
          del edge[2]['func']
          del edge[2]['path']
          del edge[2]['dflow_con_redist']
          del edge[2]['cmdline']
        else:  # nprocs == 0
          my_edge = Edge(edge[0], edge[1], edge[2]['start_proc'], edge[2]['nprocs'], edge[2]['prod_dflow_redist'])

        del edge[2]['nprocs']
        del edge[2]['start_proc']
        del edge[2]['prod_dflow_redist']
      # end else no topology
      edge[2]['edge'] = my_edge # Insert the object and clean the other attributes
      

""" Process the graph and generate the necessary files
"""
def processGraph( name, inputFile = "", filter_level = Filter_level.NONE, mpirunPath = "", mpirunOpt = "", envTarget = "generic"):
    createObjects(workflow_graph)
    check_contracts(workflow_graph, filter_level)
    checkCycles(workflow_graph)
    checkTopologyRanking(workflow_graph)
    workflowToJson(workflow_graph, name+".json", filter_level)
    workflowToSh(workflow_graph, name+".sh", inputFile, mpirunOpt = mpirunOpt, mpirunPath = mpirunPath, envTarget = envTarget)
