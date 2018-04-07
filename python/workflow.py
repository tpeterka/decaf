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

from collections import defaultdict


class Filter_level:
  NONE, PYTHON, PY_AND_SOURCE, EVERYWHERE = range(4)
  dictReverse = {
    NONE : "NONE",
    PYTHON : "PYTHON",
    PY_AND_SOURCE : "PY_AND_SOURCE",
    EVERYWHERE : "EVERYWHERE"
  }

""" Object holding information about a Node """
class Node:
  def __init__(self, name, start_proc, nprocs, func, cmdline):
    self.name = name
    self.func = func
    self.cmdline = cmdline
    self.start_proc = start_proc
    self.nprocs = nprocs
    self.tokens = 0
    self.inports = {}
    self.outports = {}

  def setTokens(self, tokens):
    self.tokens = tokens


  def addInputPort(self, name, contract=0):
    if contract == 0:
      self.inports[name] = {}
    else:
      self.inports[name] = contract.inputs


  def addInput(self, portname, key, typename, period=1):
    if not portname in self.inports:
      self.inports[portname] = {key : [typename, period]}
    else:
      self.inports[portname][key] = [typename, period]

  def addInputFromDict(self, portname, dict):
    if not portname in self.inports:
      self.inports[portname] = {}
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.inports[portname][key] = val


  def addOutputPort(self, name, contract=0):
    if contract == 0:
      self.outports[name] = {}
    else:
      self.outports[name] = contract.outputs


  def addOutput(self, portname, key, typename, period=1):
    if not portname in self.outports:
      self.outports[portname] = {key : [typename, period]}
    else:
      self.outports[portname][key] = [typename, period]

  def addOutputFromDict(self, portname, dict):
    if not portname in self.outports:
      self.outports[portname] = {}
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.outports[portname][key] = val
# End of class Node

""" To create a Node using a Topology """
def nodeFromTopo(name, func, cmdline, topology):
  n = Node(name, topology.offsetRank, topology.nProcs, func, cmdline)
  n.topology=topology
  return n


""" Object holding information about an Edge """
class Edge:
  def __init__(self, srcString, destString, start_proc, nprocs, prod_dflow_redist, func='', path='', dflow_con_redist='', cmdline=''):
    srcSplit = srcString.split('.')
    destSplit = destString.split('.')
    self.src = srcSplit[0]
    self.dest = destSplit[0]
    self.start_proc = start_proc
    self.nprocs = nprocs
    self.prod_dflow_redist = prod_dflow_redist

    if nprocs != 0 and cmdline == '':
      raise ValueError("ERROR: Missing links arguments for the Edge \"%s->%s\"." % (src, dest))

    if nprocs != 0:
      self.func = func
      self.path = path
      self.dflow_con_redist = dflow_con_redist
      self.cmdline = cmdline

    if ('.' in srcString) and ('.' in destString): #In case we have ports
      self.srcPort = srcSplit[1]
      self.destPort = destSplit[1]
    else:
      self.srcPort = ''
      self.destPort = ''

    self.inputs = {}
    self.outputs = {}

  def setAny(self, bAny): # If bAny is set to true, the runtime considers there is a ContractLink, even if inputs and outputs are empty
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", setting the boolean bAny failed, continuing" %
              (self.src, self.dest))
    else:
      self.bAny = bAny


  def addContractLink(self, cLink):
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", adding a ContractLink is useless, continuing." %
              (self.src, self.dest))
    self.inputs = cLink.inputs
    self.outputs = cLink.outputs
    self.bAny = cLink.bAny

  def addInputFromDict(self, dict):
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", adding Inputs is useless, continuing." %
              (self.src, self.dest))
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.inputs[key] = val

  def addInput(self, key, typename, period=1):
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", adding Inputs is useless, continuing." %
              (self.src, self.dest))
    self.inputs[key] = [typename, period]


  def addOutputFromDict(self, dict):
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", adding Outputs is useless, continuing." %
              (self.src, self.dest))
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.outputs[key] = val

  def addOutput(self, key, typename, period=1):
    if self.nprocs == 0:
      print("WARNING: There is no proc in the Edge \"%s->%s\", adding Outputs is useless, continuing." %
              (self.src, self.dest))
    self.outputs[key] = [typename, period]
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


# Checks the contracts between each pair of producer/consumer, taking into account the contracts on links if any
def check_contracts(graph, filter_level):
    if filter_level == Filter_level.NONE:
      return
    # TODO remove all checks of filter_level == NONE in the rest of this function (and function checkWithPorts, checkWithoutPorts also) !!!
    # Well, check if we can remove it...
      
    my_list = [] # To check if an input port is connected to only one output port
    
    for graphEdge in graph.edges(data=True):
        edge = graphEdge[2]["edge"]              # Edge object
        prod = graph.node[graphEdge[0]]["node"]  # Node object for prod
        con = graph.node[graphEdge[1]]["node"]   # Node object for con

        if edge.srcPort != '' and edge.destPort != '' : # If the prod/con of this edge have ports
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
  OutportName = edge.srcPort
  Outport = prod.outports[OutportName]

  InportName = edge.destPort
  Inport = con.inports[InportName]

  s = edge.dest+'.'+InportName
  if s in my_list: # This input port is already connected to an output port
    raise ValueError("ERROR: The Input port \"%s\" cannot be connected to more than one Output port, aborting." % (s) )
  else:
    my_list.append(s)

  if edge.nprocs != 0 and hasattr(edge,'bAny'):
    keys = {}
    keys_link = {}
    attachAny = False
    #Match contract Prod with link input
    if len(edge.inputs) == 0:
      if edge.bAny == False:
        raise ValueError("ERROR: The link of \"%s.%s->%s\" has no inputs and bAny is set to False, aborting." % (edge.src, OutportName, s))
      else:
        if len(Outport) == 0:
          keys = 0 # No need to perform filtering at runtime
        else:
          intersection_keys = {key:[Inport[key][0]] for key in Inport.keys() if (key in Outport) and ( (filter_level == Filter_level.NONE) or (Inport[key][0] == Outport[key][0]) ) }
          for key in intersection_keys.keys():
            new_val = Inport[key][1] * Outport[key][1]
            intersection_keys[key].append(new_val)
            #if new_val != Inport[key][1]: # no need to print this warning since the field is just here to be forwarded to the consumer
            #  print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, new_val, con_in[key][1])
          if len(intersection_keys) != 0:
            print("WARNING: the link of \"%s.%s->%s\" has no inputs, only fields needed by the Input port will be received" %
                    (edge.src, OutportName, s))
            keys = intersection_keys
          else: # There is nothing to sent by the producer, would block at runtime
            raise ValueError("ERROR: the link of \"%s.%s->%s\" has no inputs and there is no field in common between the Output and Input ports, aborting." % (edge.src, OutportName, s))
    else: # len(edge.inputs) != 0
      if len(Outport) == 0:
        if edge.bAny == True:
          print("WARNING: The link of \"%s.%s->%s\" will accept anything from the Output port." %
                  (edge.src, OutportName, s))
          if len(Inport) == 0:
            attachAny = True
            keys = edge.inputs
          else:
            keys = 0
        else:
          print("WARNING: The Output port of \"%s.%s->%s\" can send anything, only fields needed by the link will be kept, the periodicity may not be correct." %
                  (edge.src, OutportName, s))
          keys = edge.inputs
      else : # len(Outport) != 0
        list_fields = {}
        sk = ""
        for key, val in edge.inputs.items():
          if not key in Outport:
            sk += key + ", "
          else:
            if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]):
              new_val = val[1] * Outport[key][1]
              list_fields[key] = [val[0], new_val]
              if new_val != val[1]:
                print("WARNING: the link of \"%s.%s->%s\" will receive %s with periodicity %s instead of %s." %
                        (edge.src, OutportName, s, key, new_val, val[1]))
            else:
              raise ValueError("ERROR: the types of \"%s\" does not match for the Output port and the link of \"%s.%s->%s\", aborting." % (key, edge.src, OutportName, s))
        if sk != "":
          raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port of \"%s.%s->%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName, s))
        keys = list_fields
        if edge.bAny == True: # We complete with the other keys in Outport if they are in Inport and of the same type
          for key, val in Outport.items():
            if (not key in keys) and ( (len(Inport) == 0) or ( key in Inport and Outport[key][0] == Inport[key][0] ) ): 
              keys[key] = val

    #Match link output with contract Con
    if len(edge.outputs) == 0:
      if edge.bAny == False:
        raise ValueError("ERROR: The link of \"%s.%s->%s\" has no outputs and bAny is set to False, aborting." % (edge.src, OutportName, s))
      else:
        if len(Inport) == 0:
          keys_link = 0 # No need to perform filtering at runtime
        else: # Checks if keys of Inport are a subset of Outport
          list_fields = {}
          sk = ""
          for key, val in Inport.items():
            if not key in Outport : # The key is not sent by the producer
              sk+= key + ", "
            else:
              if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]) : # The types match, add the field to the list and update the periodicity
                new_val = val[1] * Outport[key][1]
                list_fields[key] = [val[0], new_val]
                if new_val != val[1]:
                  print("WARNING: %s will receive %s with periodicity %s instead of %s." %
                          (edge.dest, key, new_val, val[1]))
              else:
                raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

          if sk != "":
            raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
          keys_link = list_fields
          print("WARNING: The link of \"%s.%s->%s\" has no outputs, all fields needed by the Input port will be forwarded." %
                  (edge.src, OutportName, s))
    else: # len(edge.outputs) != 0
      if len(Inport) == 0:
        if edge.bAny == False:
          keys_link = edge.outputs
        else: # Need to attach keys of output link and of output Port
          print("WARNING: The link of \"%s.%s->%s\" will forward anything to the Input port." %
                  (edge.src, OutportName, s))
          if len(Outport) == 0: # If there are no contracts on Nodes but only contract on the Link
            attachAny = True
            keys_link = edge.outputs
          else:
            for key, val in edge.outputs.items():
              keys_link[key] = val
            for key, val in Outport.items():
              if not key in keys_link:
                keys_link[key] = val
      else: # len(Inport) != 0
        list_fields = {}
        tmp = {}
        for key, val in Inport.items():
          if not key in edge.outputs:
            tmp[key] = val
          else:
            if (filter_level == Filter_level.NONE) or (val[0] == edge.outputs[key][0]):
              new_val = val[1] * edge.outputs[key][1]
              list_fields[key] = [val[0], new_val]
              if new_val != val[1]:
                print("WARNING: the Input port of \"%s.%s->%s\" will receive %s with periodicity %s instead of %s." %
                        (edge.src, OutportName, s, key, new_val, val[1]))
            else:
              raise ValueError("ERROR: the types of \"%s\" does not match for the link and the Input port of \"%s.%s->%s\", aborting." % (key, edge.src, OutportName, s))
        if edge.bAny == False:
          if len(tmp) == 0:
            keys_link = list_fields
          else:
            sk = ""
            for key in tmp.keys():
              sk += key + ", "
            raise ValueError("ERROR: the fields \"%s\" are not sent by the link of \"%s.%s->%s\", aborting" % (sk.rstrip(", "), edge.src, OutportName, s))
        else: #bAny == True, need to complete with the keys sent by the Prod
          if len(Outport) == 0: # The Outport can send anything, assume the missing keys are sent
            for key, val in tmp.items():
              list_fields[key] = val
            print("WARNING: The link of \"%s.%s->%s\" will forward anything from the Output port, cannot guarantee the fields needed by the Input port will be present." %
                    (edge.src, OutportName, s))
            keys_link = list_fields
          else: # Complete with the keys of the Output port
            sk = ""
            for key, val in tmp.items():
              if not key in Outport:
                sk += key + ", "
              else:
                if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]):
                  new_val = val[1] * Outport[key][1]
                  list_fields[key] = [val[0], new_val]
                  if new_val != val[1]:
                    print("WARNING: the Input port of \"%s.%s->%s\" will receive %s with periodicity %s instead of %s." %
                            (edge.src, OutportName, s, key, new_val, val[1]))
                else:
                  raise ValueError("ERROR: the types of \"%s\" does not match for the Output port and the Input port of \"%s.%s->%s\", aborting." % (key, edge.src, OutportName, s))
            if sk != "":
              raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port of \"%s.%s->%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName, s))
            keys_link = list_fields

    return (keys, keys_link, attachAny)
  # End of if hasattr(edge,'bAny')
  else:
    if len(Inport) == 0: # The Input port accepts anything, send only keys in the output port contract
      if len(Outport) == 0:
        print("WARNING: The port \"%s\" will accept everything sent by \"%s.%s\"." %
            (s, edge.src, OutportName))
        return (0,0, False)
      else:
        return (Outport, 0, False)

    if len(Outport) == 0: # The Output port can send anything, keep only the list of fields needed by the consumer
      print("WARNING: The Output port \"%s.%s\" can send anything, only fields needed by the Input port \"%s\" will be kept, the periodicity may not be correct." %
          (edge.src, OutportName, s))
      return (Inport, 0, False)

    else: # Checks if the list of fields of the consumer is a subset of the list of fields from the producer
      list_fields = {}
      sk = ""
      for key, val in Inport.items():
        if not key in Outport : # The key is not sent by the producer
          sk+= key + ", "
        else:
          if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]) : # The types match, add the field to the list and update the periodicity
            new_val = val[1] * Outport[key][1]
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

# Apply a round robin extension of the list
# Ex: input  = hostlist= host1,host2  nprocs = 4
#     output = host1,host2,host1,host2 
def expandHostlist(hostlist, nprocs):

  if len(hostlist) >= nprocs:
    return hostlist

  newHostList = []
  for i in range(0, nprocs):
    newHostList.append(hostlist[i % len(hostlist)])

  return newHostList 

class Topology:
  """ Object holder for informations about the plateform to use """

  def __init__(self, name, nProcs = 0, hostfilename = "", offsetRank = 0, procPerNode = 0, offsetProcPerNode = 0):

      self.name = name                  # Name of the topology
      self.nProcs = nProcs                # Total number of MPI ranks in the topology
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
        if len(self.hostlist) == 0:
          raise ValueError("ERROR: the given hostfile does not contain any hostname")
        self.hostlist = expandHostlist(self.hostlist, self.nProcs)
        self.nodes = list(set(self.hostlist))   # Removing the duplicate hosts
        self.nNodes = len(self.nodes)
      else:
        self.hostlist = ["localhost"] * self.nProcs # should be the same as the 3 above lines
        self.nodes = ["localhost"]
        self.nNodes = 1

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
# End of class Topology


  def splitTopologyByDict(self, info):

      sum = 0

      for item in info:
          sum = sum + item['nprocs']

      # Enough rank check
      if sum > self.nProcs:
        raise ValueError("Not enough rank available. Asked %s, given %s." % (sum, self.nProcs))

      offset = 0
      splits = []

      for item in info:
        subTopo = Topology(item['name'], item['nprocs'], offsetRank = offset)

        subTopo.hostlist = self.hostlist[offset:offset+item['nprocs']]
        subTopo.nodes = list(set(subTopo.hostlist))
        subTopo.nNodes = len(subTopo.nodes)
        offset += item['nprocs']

        splits.append(subTopo)

      return splits

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
                    if val[1]['node'].tokens > 0:
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
                                          "func" : node.func,
                                          "tokens" : node.tokens})


        val[1]['index'] = i
        i += 1


    data["workflow"]["edges"] = []
    i = 0
    for graphEdge in graph.edges(data=True):
        edge = graphEdge[2]["edge"]
        prod_id  = graph.node[graphEdge[0]]['index']
        con_id   = graph.node[graphEdge[1]]['index']
        data["workflow"]["edges"].append({"start_proc" : edge.start_proc,
                                          "nprocs" : edge.nprocs,
                                          "source" : prod_id,
                                          "target" : con_id,
                                          "prod_dflow_redist" : edge.prod_dflow_redist,
                                          "name" : graphEdge[0] + "_" + graphEdge[1]})

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
        if edge.srcPort != '' and edge.destPort != '':
          data["workflow"]["edges"][i].update({"sourcePort":edge.srcPort,
                                              "targetPort":edge.destPort})

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

    for val in graph.nodes(data=True):
        node = val[1]["node"]
        if node.start_proc == rank:
            return ('node', node)

    for val in graph.edges(data=True):
        edge = val[2]["edge"]
        if (edge.start_proc == rank) and (edge.nprocs != 0):
            return ('edge', edge)

    return ('notfound', 0) # should be the same since second value is not used in this case


def workflowToSh(graph, outputFile, mpirunOpt = "", mpirunPath = ""):
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
    print("Generating bash command script "+outputFile)

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
        if hasattr(exe, 'topology') and len(exe.topology.hostlist) > 0:
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
          if hasattr(exe, 'topology') and len(exe.topology.hostlist) > 0:
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
        if hasattr(exe, 'topology') and len(exe.topology.hostlist) > 0:
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
          if hasattr(exe, 'topology') and len(exe.topology.hostlist) > 0:
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

""" Used for the retrocompatibility when not using Node and Edge objects in the graph
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
def processGraph(graph, name, filter_level = Filter_level.NONE, mpirunPath = "", mpirunOpt = ""):
    createObjects(graph)
    check_contracts(graph, filter_level)
    checkCycles(graph)
    workflowToJson(graph, name+".json", filter_level)
    workflowToSh(graph, name+".sh", mpirunOpt = mpirunOpt, mpirunPath = mpirunPath)
