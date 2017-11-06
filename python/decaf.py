# converts an nx graph into a workflow data structure and runs the workflow

import imp
import sys
import os
import exceptions
import getopt
import argparse
import json
import networkx as nx

from collections import defaultdict

# General graph hosting the entire workflow structure.
# Used to perform graph analysis and generate the json file
workflow_graph = nx.DiGraph()


class Topology:
  """ Object holder for informations about the plateform to use """

  def __init__(self, name, nProcs = 0, hostfilename = "", offsetRank = 0, procPerNode = 0, offsetProcPerNode = 0):

      self.name = name                  # Name of the topology
      self.nProcs = nProcs              # Total number of MPI ranks in the topology
      self.hostfilename = hostfilename  # File containing the list of hosts. Must match  totalProc
      self.hostlist = []                # List of hosts.
      self.offsetRank = offsetRank      # Rank of the first rank in this topology
      self.nNodes = 0                   # Number of nodes
      self.nodes = []                   # List of nodes
      self.procPerNode = procPerNode    # Number of MPI ranks per node. If not given, deduced from the host list
      self.offsetProcPerNode = offsetProcPerNode    # Offset of the proc ID to start per node with process pinning


      if hostfilename != "":
        f = open(hostfilename, "r")
        content = f.read()
        content = content.rstrip(' \t\n\r') #Clean the last character, usually a \n
        self.hostlist = content.split('\n')
        self.nodes = list(set(self.hostlist))   # Removing the duplicate hosts
        self.nNodes = len(self.nodes)
      else:
        self.hostlist = ["localhost"] * self.nProcs # should be the same as the 3 above lines
        self.nodes = ["localhost"]
        self.nNodes = 1

      if len(self.hostlist) != self.nProcs:
          raise ValueError("The number of hosts does not match the number of procs.")
  # End of constructor

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
      if procOffset + nProcs > self.nProcs:
        raise ValueError("Not enough rank available. Asked %s, given %s." % (procOffset + nProcs, self.nProcs))

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
        raise ValueError("Not enough rank available. Asked %s, given %s." % (sum(nProcs), self.nProcs))

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
# End of class Topology

def initParserForTopology(parser):
    """ Add the necessary arguments to initialize a topology.
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
    return Topology("root", args.nprocs, hostfilename = args.hostfile)


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
      print self.contract


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


  def setContractLink(self, cLink):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding a ContractLink is useless, continuing." % (self.src, self.dest)
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
    
    for graphEdge in graph.edges_iter(data=True):
        edge = graphEdge[2]["edge"]              # Edge object
        prod = graph.node[graphEdge[0]]["node"]  # Node object for prod
        con = graph.node[graphEdge[1]]["node"]   # Node object for con

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
            print "WARNING: the link of \"%s.%s->%s.%s\" has no input contract, only fields needed by the input port of the consumer will be received" % (edge.src, OutportName, edge.dest, InportName)
            keys = intersection_keys
          else: # There is nothing to sent by the producer, would block at runtime
            raise ValueError("ERROR: the link of \"%s.%s->%s.%s\" has no inputs and there is no field in common between the Output and Input ports, aborting." % (edge.src, OutportName, edge.dest, InportName))

    # Case with an input contract on the link
    else: # len(edge.inputs) != 0
      if len(OutportContract) == 0:
        if Link.bAny == True:
          print "WARNING: The link of \"%s.%s->%s.%s\" will accept anything from the Output port." % (edge.src, OutportName, edge.dest, InportName)
          if len(InportContract) == 0:
            attachAny = True
            keys = Link.inputs
          else:
            keys = 0
        else:
          print "WARNING: The Output port of \"%s.%s->%s.%s\" can send anything, only fields needed by the link will be kept, the periodicity may not be correct." % (edge.src, OutportName, edge.dest, InportName)
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
                print "WARNING: the link of \"%s.%s->%s.%s\" will receive %s with periodicity %s instead of %s." % (edge.src, OutportName, edge.dest, InportName, key, new_val, val[1])
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
                  print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, new_val, val[1])
              else:
                raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

          if sk != "":
            raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
          keys_link = list_fields
          print "WARNING: The link of \"%s.%s->%s.%s\" has no output contract, all fields needed by the Input port of the consumer will be forwarded." % (edge.src, OutportName, edge.dest, InportName)
    else: # len(edge.outputs) != 0
      if len(InportContract) == 0:
        if Link.bAny == False:
          keys_link = Link.outputs
        else: # Need to attach keys of output link and of output Port
          print "WARNING: The link of \"%s.%s->%s.%s\" will forward anything to the Input port." % (edge.src, OutportName, edge.dest, InportName)
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
                print "WARNING: the Input port of \"%s.%s->%s.%s\" will receive %s with periodicity %s instead of %s." % (edge.src, OutportName, edge.dest, InportName, key, new_val, val[1])
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
            print "WARNING: The link of \"%s.%s->%s.%s\" will forward anything from the Output port, cannot guarantee the fields needed by the Input port will be present." % (edge.src, OutportName, edge.dest, InportName)
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
                    print "WARNING: the Input port of \"%s.%s->%s.%s\" will receive %s with periodicity %s instead of %s." % (edge.src, OutportName, edge.dest, InportName, key, new_val, val[1])
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
        print "WARNING: The port \"%s\" will accept everything sent by \"%s.%s\"." % (InportName, edge.src, OutportName)
        return (0,0, False)
      else:
        return (OutportContract, 0, False)

    if len(OutportContract) == 0: # The Output port can send anything, keep only the list of fields needed by the consumer
      print "WARNING: The Output port \"%s.%s\" can send anything, only fields needed by the Input port \"%s\" will be kept, the periodicity may not be correct." % (edge.src, OutportName, InportName)
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
              print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, new_val, val[1])
          else:
            raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

      if sk != "":
        raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
    return (list_fields, 0, False) # This is the dict of fields to be exchanged in this edge; A field is an entry of a dict: name:[type, period]
# end of checkWithPorts



def checkCycles(graph):

  cycle_list = list(nx.simple_cycles(graph))
  if len(cycle_list) > 0:
    print "Decaf detected " + str(len(cycle_list)) + " cycles. Checking for tokens..."

  for cycle in cycle_list:
    # Checking if one of the node in the cycle has at least 1 token
    found_token = False
    for node in cycle:

      # The list given by simple_cycles does not have the attribute
      # We have to find back the correct node
      for val in graph.nodes_iter(data=True):
        if val[0] == node:
          for name,port in val[1]['node'].inports.iteritems():
            if port.tokens > 0:
              found_token = True
    if found_token:
      print "Token detected in the cycle."
    else:
      raise ValueError("A cycle was detected within the cycle %s, but no token were inserted. Please insert at least one token in one of the cycle's tasks." % (cycle))





def workflowToJson(graph, outputFile, filter_level):
    print "Generating graph description file "+outputFile

    nodes   = []
    links   = []
    list_id = []

    data = {}
    data["workflow"] = {}
    data["workflow"]["filter_level"] = Filter_level.dictReverse[filter_level]

    data["workflow"]["nodes"] = []
    i = 0
    for val in graph.nodes_iter(data=True):
        node = val[1]["node"]
        data["workflow"]["nodes"].append({"start_proc" : node.start_proc,
                                          "nprocs" : node.nprocs,
                                          "func" : node.func})


        val[1]['index'] = i
        i += 1


    data["workflow"]["edges"] = []
    i = 0
    for graphEdge in graph.edges_iter(data=True):
        edge = graphEdge[2]["edge"]
        prod_id  = graph.node[graphEdge[0]]['index']
        con_id   = graph.node[graphEdge[1]]['index']
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

        # TODO put stream and others in the Edge object?
        if 'stream' in graphEdge[2]:
            data["workflow"]["edges"][i]["stream"] = graphEdge[2]['stream']
            data["workflow"]["edges"][i]["frame_policy"] = graphEdge[2]['frame_policy']
            data["workflow"]["edges"][i]["storage_types"] = graphEdge[2]['storage_types']
            data["workflow"]["edges"][i]["storage_collection_policy"] = graphEdge[2]['storage_collection_policy']
            data["workflow"]["edges"][i]["max_storage_sizes"] = graphEdge[2]['max_storage_sizes']
            if 'prod_output_freq' in graphEdge[2]:
              data["workflow"]["edges"][i]["prod_output_freq"] = graphEdge[2]['prod_output_freq']
            if 'low_output_freq' in graphEdge[2]:
              data["workflow"]["edges"][i]["low_output_freq"] = graphEdge[2]['low_output_freq']
            if 'high_output_freq' in graphEdge[2]:
              data["workflow"]["edges"][i]["high_output_freq"] = graphEdge[2]['high_output_freq']

        # If there are ports for this edge
        #if edge.srcPort != '' and edge.destPort != '':
        #  data["workflow"]["edges"][i].update({"sourcePort":edge.srcPort,
        #                                      "targetPort":edge.destPort})


        i += 1


    f = open(outputFile, 'w')
    json.dump(data, f, indent=4)
    f.close()
# End workflowToJson

# Add streaming information to a link
def updateLinkStream(graph, prod, con, stream, frame_policy, storage_collection_policy, storage_types, max_storage_sizes, prod_output_freq):
    graph.edge[prod][con]['stream'] = stream
    graph.edge[prod][con]['frame_policy'] = frame_policy
    graph.edge[prod][con]['storage_collection_policy'] = storage_collection_policy
    graph.edge[prod][con]['storage_types'] = storage_types
    graph.edge[prod][con]['max_storage_sizes'] = max_storage_sizes
    graph.edge[prod][con]['prod_output_freq'] = prod_output_freq

# Shortcut function to define a sequential streaming dataflow
def addSeqStream(graph, prod, con, buffers = ["mainmem"], max_buffer_sizes = [10], prod_output_freq = 1):
    updateLinkStream(graph, prod, con, 'double', 'seq', 'greedy', buffers, max_buffer_sizes, prod_output_freq)

# Shortcut function to define a stream sending the most recent frame to the consumer
def addMostRecentStream(graph, prod, con, buffers = ["mainmem"], max_buffer_sizes = [10], prod_output_freq = 1):
    updateLinkStream(graph, prod, con, 'single', 'recent', 'lru', buffers, max_buffer_sizes, prod_output_freq)

def addLowHighStream(graph, prod, con, low_freq, high_freq, buffers = ["mainmem"], max_buffer_sizes = [10]):
    updateLinkStream(graph, prod, con, 'single', 'lowhigh', 'greedy', buffers, max_buffer_sizes, 1)
    graph.edge[prod][con]['low_output_freq'] = low_freq
    graph.edge[prod][con]['high_output_freq'] = high_freq

def addDirectSyncStream(graph, prod, con, prod_output_freq = 1):
    updateLinkStream(graph, prod, con, 'single', 'seq', 'greedy', [], [], prod_output_freq)

# Looking for a node/edge starting at a particular rank
def getNodeWithRank(rank, graph):

    for val in graph.nodes_iter(data=True):
        node = val[1]["node"]
        if node.start_proc == rank:
            return ('node', node)

    for val in graph.edges_iter(data=True):
        edge = val[2]["edge"]
        if (edge.start_proc == rank) and (edge.nprocs != 0):
            return ('edge', edge)

    #return ('notfound', graph.nodes_iter())
    return ('notfound', 0) # should be the same since second value is not used in this case


def workflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
    print "Selecting the transport method..."

    transport = ""
    for graphEdge in graph.edges_iter(data=True):
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
        print "Selected transport method: None"
    else:
        print "Selected transport method: "+transport

    if graph.number_of_edges() == 0:
        MPIworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport == "mpi":
      MPIworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport == "cci":
      MPMDworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    elif transport == "file":
      MPMDworkflowToSh(graph,outputFile,mpirunOpt,mpirunPath)
    else:
      raise ValueError("ERROR: Unknow transport method selected %s" % (transport))

# Build the mpirun command in MPMD mode
# Parse the graph to sequence the executables with their
# associated MPI ranks and arguments
def MPIworkflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
    print "Generating bash command script "+outputFile

    currentRank = 0
    hostlist = []
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" --hostfile hostfile_workflow.txt "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()

    # Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print 'ERROR: Unable to find an executable for the rank ' + str(rank)
        exit()

      if type == 'node':

        if exe.nprocs == 0:
          print 'ERROR: a node can not have 0 MPI rank.'
          exit()

        mpirunCommand += "-np "+str(exe.nprocs)

        #Checking if a topology is specified. If no, fill a filehost with localhost
        if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
          for host in exe.topology.hostlist:
            hostlist.append(host)
        else:
            #print "No topology found."
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")
        mpirunCommand += " "+str(exe.cmdline)+" : "
        currentRank += exe.nprocs

      if type == 'edge':

        if exe.nprocs != 0:
          mpirunCommand += "-np "+str(exe.nprocs)

          #Checking if a topology is specified. If no, fill a filehost with localhost
          if hasattr(exe, 'topology') and exe.topology != None and len(exe.topology.hostlist) > 0:
            for host in exe.topology.hostlist:
              hostlist.append(host)
          else:
            for j in range(0, exe.nprocs):
              hostlist.append("localhost")
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
    

"""Build the various mpirun commands for each executable
   The function generate the environment variables necesaries for Decaf
   to recreate the ranking of the workflow
"""
def MPMDworkflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
    print "Generating bash command script for MPMD "+outputFile

    currentRank = 0
    totalRank = 0
    mpirunCommand = mpirunPath+"mpirun "+mpirunOpt+" "
    nbExecutables = graph.number_of_nodes() + graph.number_of_edges()

    commands = []

    # Computing the total number of ranks with the workflow
    for val in graph.nodes_iter(data=True):
        node = val[1]["node"]
        totalRank+=node.nprocs


    for graphEdge in graph.edges_iter(data=True):
        edge = graphEdge[2]["edge"]
        totalRank+=edge.nprocs

    #print "Total number of ranks in the workflow: "+str(totalRank)

    #Parsing the graph looking at the current rank
    for i in range(0, nbExecutables):
      hostlist = []
      (type, exe) = getNodeWithRank(currentRank, graph)
      if type == 'none':
        print 'ERROR: Unable to find an executable for the rank ' + str(rank)
        exit()

      hostfilename = "hostfile_task"+str(i)
      if type == 'node':

        if exe.nprocs == 0:
          print 'ERROR: a node can not have 0 MPI rank.'
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
  for node in graph.nodes_iter(data=True):
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
      
  for edge in graph.edges_iter(data=True):
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
def processGraph( name, filter_level = Filter_level.NONE, mpirunPath = "", mpirunOpt = ""):
    createObjects(workflow_graph)
    check_contracts(workflow_graph, filter_level)
    checkCycles(workflow_graph)
    workflowToJson(workflow_graph, name+".json", filter_level)
    workflowToSh(workflow_graph, name+".sh", mpirunOpt = mpirunOpt, mpirunPath = mpirunPath)
