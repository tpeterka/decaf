# converts an nx graph into a workflow data structure and runs the workflow

import imp
import sys
import os
import exceptions
import getopt
import argparse
import json

from collections import defaultdict

import fractions  #Definition of the least common multiple of two numbers; Used for the period in Contracts
def lcm(a,b): return abs(a * b) / fractions.gcd(a,b) if a and b else 0

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
    self.inports = {}
    self.outports = {}


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

  def addInputFromContract(self, portname, contract):
    if contract.inputs == {}:
      raise ValueError("ERROR in addInputFromContract: the contract does not have inputs.")
    self.addInputFromDict(self, portname, contract.inputs)


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

  def addOutputFromContract(self, portname, contract):
    if contract.outputs == {}:
      raise ValueError("ERROR in addOutputFromContract: the contract does not have outputs.")
    self.addOutputFromDict(self, portname, contract.outputs)
# End of class Node

""" To create a Node using a Topology """
def nodeFromTopo(name, func, cmdline, topology):
  n = Node(name, topology.offsetRank, topology.nProcs, func, cmdline)
  n.topology=topology
  return n


""" Object holding information about an Edge """
class Edge:
  def __init__(self, src, dest, start_proc, nprocs, prod_dflow_redist, func='', path='', dflow_con_redist='', cmdline=''):
    srcSplit = src.split('.')
    destSplit = dest.split('.')
    self.src = srcSplit[0]
    self.dest = destSplit[0]
    self.start_proc = start_proc
    self.nprocs = nprocs
    self.prod_dflow_redist = prod_dflow_redist

    if nprocs != 0 and func == '':
      raise ValueError("ERROR: Missing links arguments for the Edge \"%s->%s\"." % (src, dest))

    if nprocs != 0:
      self.func = func
      self.path = path
      self.dflow_con_redist = dflow_con_redist
      self.cmdline = cmdline

    if ('.' in src) and ('.' in dest): #In case we have ports
      self.srcPort = srcSplit[1]
      self.destPort = destSplit[1]
    else:
      self.srcPort = ''
      self.destPort = ''

    self.contract = 0

  def addContractLink(self, contractLink):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding a contractLink is useless, continuing." % (self.src, self.dest)
    self.contract = contractLink

  def addContractFromDict(self, dict):
    if self.contract == 0:
      self.contract = ContractLink()
    for key, val in dict.items():
      self.contract.dict[key] = val
# End of class Edge

""" To create an Edge with a Topology """
def edgeFromTopo(src, dest, topology, prod_dflow_redist, func='', path='', dflow_con_redist='', cmdline=''):
    e = Edge(src, dest, topology.offsetRank, topology.nProcs, prod_dflow_redist, func, path, dflow_con_redist, cmdline)
    e.topology = topology
    return e


def addNode(graph, n):
  if hasattr(n, 'topology'):
    graph.add_node(n.name, topology=n.topology, func=n.func, cmdline=n.cmdline, inports=n.inports, outports=n.outports)
  else:
    graph.add_node(n.name, nprocs=n.nprocs, start_proc=n.start_proc, func=n.func, cmdline=n.cmdline, inports=n.inports, outports=n.outports)


def addEdge(graph, e):
  if hasattr(e, 'topology'):
    if hasattr(e, 'srcPort'):
      if e.topology.nProcs != 0:
        graph.add_edge(e.src, e.dest, prodPort = e.srcPort, conPort=e.destPort, topology=e.topology, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist,
                func=e.func, path=e.path, dflow_con_redist=e.dflow_con_redist, cmdline=e.cmdline)
      else:
        graph.add_edge(e.src, e.dest, prodPort = e.srcPort, conPort=e.destPort, topology=e.topology, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist)

    else:
      if e.topology.nProcs != 0:
        graph.add_edge(e.src, e.dest, topology=e.topology, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist,
                func=e.func, path=e.path, dflow_con_redist=e.dflow_con_redist, cmdline=e.cmdline)
      else:
        graph.add_edge(e.src, e.dest, topology=e.topology, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist)
  else:
    if hasattr(e, 'srcPort'):
      if e.nprocs != 0:
        graph.add_edge(e.src, e.dest, prodPort = e.srcPort, conPort=e.destPort, nprocs=e.nprocs, start_proc=e.start_proc, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist,
                func=e.func, path=e.path, dflow_con_redist=e.dflow_con_redist, cmdline=e.cmdline)
      else:
        graph.add_edge(e.src, e.dest, prodPort = e.srcPort, conPort=e.destPort, nprocs=e.nprocs, start_proc=e.start_proc, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist)

    else:
      if e.nprocs != 0:
        graph.add_edge(e.src, e.dest, nprocs=e.nprocs, start_proc=e.start_proc, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist,
                func=e.func, path=e.path, dflow_con_redist=e.dflow_con_redist, cmdline=e.cmdline)
      else:
        graph.add_edge(e.src, e.dest, nprocs=e.nprocs, start_proc=e.start_proc, contractLink = e.contract, prod_dflow_redist=e.prod_dflow_redist)


""" Object for contract associated to a link """
"""class ContractLink:
  def __init__(self, dict_ = {}):#, bany = False):
    self.dict = dict_ # Dictionary of keys to exchange
    #self.bAny = bany  # Accept anything or not? #TODO Is this really useful ?
    #If bAny set to True, the content of dict is required but anything else will be forwarded as well

  def addKey(self, key, typename):
    self.dict[key] = typeame

  def addKeyFromDict(self, dict):
    for key, val in dict.items():
      self.dict[key] = val
#End class ContractEdge
"""

""" Object holding information about input/output data name, type and periodicity """
class Contract:
  def __init__(self, jsonfilename = ""):
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

# End of class Contract

# Checks if the input port (consumer) contract of an edge is a subset of the output port (producer) contract
def check_contracts(graph, filter_level):
    if filter_level == Filter_level.NONE:
      return
      
    my_list = [] # To check if an input port is connected to only one output port

    dict = defaultdict(set)
    
    for edge in graph.edges_iter(data=True):
        prod = graph.node[edge[0]]
        con = graph.node[edge[1]]
        
        if 'prodPort' in edge[2] and edge[2]['prodPort'] != '' : # If the prod/con of this edge have ports          
          OutportName = edge[2]['prodPort']
          Outport = graph.node[edge[0]]['outports'][OutportName]

          InportName = edge[2]['conPort']
          Inport = graph.node[edge[1]]['inports'][InportName]

          s = edge[1]+'.'+InportName
          if s in my_list: # This input port is already connected to an output port
            raise ValueError("ERROR: The port \"%s\" cannot be connected to more than one Output port, aborting." % (s) )
          else:
            my_list.append(s)

          if len(Inport) == 0: # The Input port accepts anything, no need to perform check or filtering
            print "WARNING: The port \"%s\" will accept anything sent by \"%s.%s\" and the periodicity may not be correct, continuing." % (s, edge[0], OutportName)
            continue

          if len(Outport) == 0: # The Output port can send anything, keep only the list of fields needed by the consumer
          #TODO in this case, must perform the checks at runtime at the dataflow->get, need to find a way to enforce that
            print "WARNING: The output port \"%s.%s\" can send anything, only fields needed by the port \"%s\" will be kept, the periodicity may not be correct, continuing." % (edge[0], OutportName, s)
            list_fields = Inport

          else: # Checks if the list of fields of the consumer is a subset of the list of fields from the producer
            list_fields = {}
            s = ""
            for key, val in Inport.items():
              if not key in Outport : # The key is not sent by the producer
                s+= key + " "
              else:
                if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]) : # The types match, add the field to the list and update the periodicity
                  lcm_val = lcm(val[1], Outport[key][1]) # Periodicity for this field is the Least Common Multiple b/w the periodicity of the input and output contracts
                  list_fields[key] = [val[0], lcm_val]
                  if lcm_val != val[1]:
                    print "WARNING: %s will receive %s with periodicity %s instead of %s, continuing" % (edge[1], key, lcm_val, val[1])
                else:
                  raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\"." % (key, edge[0], OutportName, edge[1], InportName))

            if s != "":
              raise ValueError("ERROR: the keys \"%s\" are not sent in the output port \"%s.%s\"." % (s.rstrip(), edge[0], OutportName))

          # We add the dict of fields to be exchanged in this edge; A field is an entry of a dict: name:[type, period]
          edge[2]['keys'] = list_fields
        #End if
        else: # This edge does not deal with ports
        #This is the old checking of keys/types with contracts without ports
          if ("contract" in prod) and ("contract" in con):
            if prod["contract"].outputs == {}:
                raise ValueError("ERROR while checking contracts: %s has no outputs" % edge[0])
            else:
                prod_out = prod["contract"].outputs

            if con["contract"].inputs == {}:
                raise ValueError("ERROR while checking contracts: %s has no inputs" % edge[1])
            else:
                con_in = con["contract"].inputs

            # We add for each edge the list of pairs key/type which is the intersection of the two contracts
            intersection_keys = {key:[con_in[key][0]] for key in con_in.keys() if (key in prod_out) and ( (filter_level == Filter_level.NONE) or (con_in[key][0] == prod_out[key][0]) ) }
            for key in intersection_keys.keys():
                dict[edge[1]].add(key)
                lcm_val = lcm(con_in[key][1], prod_out[key][1])
                intersection_keys[key].append(lcm_val)
                if(lcm_val != con_in[key][1]):
                  print "WARNING: %s will receive %s with periodicity %s instead of %s, continuing" % (edge[1], key, lcm_val, con_in[key][1])

            if len(intersection_keys) == 0:
                raise ValueError("ERROR intersection of keys from %s and %s is empty" % (edge[0], edge[1]))

            # We add the list of pairs key/type to be exchanged b/w the producer and consumer
            edge[2]['keys'] = intersection_keys
        #End else
        """ TODO work in progress here
        # Here we check if there is a contractLink and if it is a subset of the general contract of the edge
        # TODO VERIFY THIS BEHAVIOR, does it do as planned ?
        if 'contractLink' in edge[2] and edge[2]['contractLink'] != 0:
          s = ""
          for key, val in edge[2]['contractLink'].dict.items():
            if not key in edge[2]['keys']:
              s+= key + " "
            else:
              if(filter_level != Filter_level.NONE) and (val != edge[2]['keys'][key][0]) : #If the types does not match
                raise ValueError("ERROR: the types of \"%s\" does not match for the contract and the contractLink of \"%s.%s\" and \"%s.%s\"." % (key, edge[0], OutportName, edge[1], InportName))

          if s != "":
            raise ValueError("ERROR: the keys \"%s\" required by the link are not sent in the output port \"%s.%s\"." % (s.rstrip(), edge[0], OutportName))
        """
        
    for name, set_k in dict.items():
      needed = graph.node[name]['contract'].inputs
      received = list(set_k)
      complement = [item for item in needed if item not in received]
      if len(complement) != 0:
        s = "ERROR %s does not receive the following:" % name
        for key in complement:
          s+= " %s:%s," %(key, needed[key])
        #print s.rstrip(',')
        raise ValueError(s.rstrip(','))
# End of function check_contracts


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
    return Topology("global", args.nprocs, hostfilename = args.hostfile)

def processTopology(graph):
    """ Check all nodes and edge if a topology is present.
        If yes, fill the fields start_proc and nprocs 
    """
    for node in graph.nodes_iter(data=True):
        if 'topology' in node[1]:
            topo = node[1]['topology']
            node[1]['start_proc'] = topo.offsetRank
            node[1]['nprocs'] = topo.nProcs
            # Clear the graph
            del node[1]['topology'] #TODO is it really useful to remove this ? Not real memory consumption

    for edge in graph.edges_iter(data=True):
        if 'topology' in edge[2]:
            topo = edge[2]['topology']
            edge[2]['start_proc'] = topo.offsetRank
            edge[2]['nprocs'] = topo.nProcs
            # Clear the graph
            del edge[2]['topology'] #TODO is it really useful to remove this ? Not real memory consumption


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
    for node in graph.nodes_iter(data=True):
        data["workflow"]["nodes"].append({"start_proc" : node[1]['start_proc'],
                                          "nprocs" : node[1]['nprocs'],
                                          "func" : node[1]['func']})
        node[1]['index'] = i
        i += 1
    

    data["workflow"]["edges"] = []
    i = 0
    for edge in graph.edges_iter(data=True):
        prod  = graph.node[edge[0]]['index']
        con   = graph.node[edge[1]]['index']
        data["workflow"]["edges"].append({"start_proc" : edge[2]['start_proc'],
                                          "nprocs" : edge[2]['nprocs'],
                                          "source" : prod,
                                          "target" : con,
                                          "prod_dflow_redist" : edge[2]['prod_dflow_redist']})

        if edge[2]["nprocs"] != 0:
          data["workflow"]["edges"][i].update({"func" : edge[2]['func'],
                                          "dflow_con_redist" : edge[2]['dflow_con_redist'],
                                          "path" : edge[2]['path']})
         
        # TODO VERIFY THIS, used with link any + contract 
        if "linkContract" in edge[2]:
          data["workflow"]["edges"][i]["linkContract"] = edge[2]["linkContract"].dict
        
        if "keys" in edge[2]:
          data["workflow"]["edges"][i]["keys"] = edge[2]["keys"]

        if "prodPort" in edge[2] and edge[2]["prodPort"] != '':
          data["workflow"]["edges"][i].update({"sourcePort":edge[2]['prodPort'],
                                              "targetPort":edge[2]['conPort']})

        i += 1


    f = open(outputFile, 'w')
    json.dump(data, f, indent=4)
    f.close()


# Looking for a node/edge starting at a particular rank
def getNodeWithRank(rank, graph):

    for node in graph.nodes_iter(data=True):
        if node[1]['start_proc'] == rank:
            return ('node', node)

    for edge in graph.edges_iter(data=True):
        if (edge[2]['start_proc'] == rank) and (edge[2]['nprocs'] != 0):
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

        if exe[2]['nprocs'] != 0:
          mpirunCommand += "-np "+str(exe[2]['nprocs'])
          mpirunCommand += " "+str(exe[2]['cmdline'])+" : "
          currentRank += exe[2]['nprocs']

    # Writting the final file
    f = open(outputFile, "w")
    content = ""
    content +="#! /bin/bash\n\n"
    content +=mpirunCommand
    content = content.rstrip(" : ") + "\n"

    f.write(content)
    f.close()
    os.system("chmod a+rx "+outputFile)

""" Process the graph and generate the necessary files
"""
def processGraph(graph, name, filter_level = Filter_level.NONE, mpirunPath = "", mpirunOpt = ""):
    check_contracts(graph, filter_level)
    processTopology(graph)
    workflowToJson(graph, name+".json", filter_level)
    workflowToSh(graph, name+".sh", mpirunOpt = mpirunOpt, mpirunPath = mpirunPath)
