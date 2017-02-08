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
    self.topology = 0  # To have information about the list of hosts and nodes of the Topology


  def addInPort(self, name, contract=0):
    if contract == 0:
      self.inports[name] = {}
    else:
      self.inports[name] = contract.inputs

  def addInput(self, portname, key, typename, period=1):
    if not portname in self.inports:
      raise ValueError("ERROR in addInput: the IN port %s is not present in %s." % (portname, self.name))
    self.inports[portname][key] = [typename, period]

  def addInputFromDict(self, portname, dict):
    if not portname in self.inports:
      raise ValueError("ERROR in addInputFromDict: the IN port %s is not present in %s." % (portname, self.name))
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.inports[portname][key] = val

  def addOutPort(self, name, contract=0):
    if contract == 0:
      self.outports[name] = {}
    else:
      self.outports[name] = contract.outputs

  def addOutput(self, portname, key, typename, period=1):
    if not portname in self.outports:
      raise ValueError("ERROR in addOutput: the OUT port %s is not present in %s." % (portname, self.name))
    self.outports[portname][key] = [typename, period]

  def addOutputFromDict(self, portname, dict):
    if not portname in self.outports:
      raise ValueError("ERROR in addOutputFromDict: the OUT port %s is not present in %s." % (portname, self.name))
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.outports[portname][key] = val

# End of class Node

""" To create a Node using a Topology """
def nodeFromTopo(name, func, cmdline, topo):
  n = Node(name, topo.offsetRank, topo.nProcs, func, cmdline)
  n.topology=topo
  return n


def addNode(graph, n):
  if n.topology == 0:
    graph.add_node(n.name, nprocs=n.nprocs, start_proc=n.start_proc, func=n.func, cmdline=n.cmdline, inports=n.inports, outports=n.outports)
  else:
    graph.add_node(n.name, topology=n.topology, func=n.func, cmdline=n.cmdline, inports=n.inports, outports=n.outports)


def addEdgeWithTopo(graph, prodString, conString, topology, prod_dflow_redist, func='', path='', dflow_con_redist='', cmdline=''):
  prod, prodP = prodString.split('.')
  con, conP = conString.split('.')
  graph.add_edge(prod, con, prodPort=prodP, conPort=conP, topology=topology, prod_dflow_redist=prod_dflow_redist, func=func, path=path, dflow_con_redist=dflow_con_redist, cmdline=cmdline)


def addEdge(graph, prodString, conString, start_proc, nprocs, prod_dflow_redist, func='', path='', dflow_con_redist='', cmdline=''):
  if nprocs != 0 and func == '':
    raise ValueError("ERROR: Missing links arguments for the edge \"%s->%s\", aborting.")
  prod, prodP = prodString.split('.')
  con, conP = conString.split('.')
  graph.add_edge(prod, con, prodPort=prodP, conPort=conP, nprocs=nprocs, start_proc=start_proc, prod_dflow_redist=prod_dflow_redist, func=func, path=path, dflow_con_redist=dflow_con_redist, cmdline=cmdline)




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
def check_contracts(graph, check_types):
    my_list = [] # To check if an input port is connected to only one output port

    dict = defaultdict(set)
    
    for edge in graph.edges_iter(data=True):
        prod = graph.node[edge[0]]
        con = graph.node[edge[1]]
        
        if 'prodPort' in edge[2]: # If the prod/con of this edge have ports          
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
            continue

          if len(Outport) == 0: # The Output port can send anything, keep only the list of fields needed by the consumer
            list_fields = Inport          

          else: # Checks if the list of fields of the consumer is a subset of the list of fields from the producer
            list_fields = {}
            s = ""
            for key, val in Inport.items():
              if not key in Outport : # The key is not sent by the producer
                s+= key + " "
              else:
                if (check_types == 0) or (val[0] == Outport[key][0]) : # The types match, add the field to the list and update the periodicity
                  lcm_val = lcm(val[1], Outport[key][1]) # Periodicity for this field is the Least Common Multiple b/w the periodicity of the input and output contracts
                  list_fields[key] = [val[0], lcm_val]
                else:
                  raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge[0], OutportName, edge[1], InportName))

            if s != "":
              raise ValueError("ERROR: the keys \"%s\"are not sent in the output port \"%s.%s\", aborting." % (s, edge[0], OutportName))

          # We add the dict of fields to be exchanged in this edge; A field is an entry of a dict: name:[type, period]
          edge[2]['keys'] = list_fields

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
            intersection_keys = {key:[con_in[key][0]] for key in con_in.keys() if (key in prod_out) and ( (check_types == 0) or (con_in[key][0] == prod_out[key][0]) ) }
            for key in intersection_keys.keys():
                dict[edge[1]].add(key)
                lcm_val = lcm(con_in[key][1], prod_out[key][1])
                intersection_keys[key].append(lcm_val)
                if(lcm_val != con_in[key][1]):
                  print "WARNING: %s will receive %s with periodicity %s instead of %s. Continuing" % (edge[1], key, lcm_val, con_in[key][1])

            if len(intersection_keys) == 0:
                raise ValueError("ERROR intersection of keys from %s and %s is empty" % (edge[0], edge[1]))

            # We add the list of pairs key/type to be exchanged b/w the producer and consumer
            edge[2]['keys'] = intersection_keys


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

    for edge in graph.edges_iter(data=True):
        if 'topology' in edge[2]:
            topo = edge[2]['topology']
            edge[2]['start_proc'] = topo.offsetRank
            edge[2]['nprocs'] = topo.nProcs


def workflowToJson(graph, outputFile, check_types):
    print "Generating graph description file "+outputFile

    nodes   = []
    links   = []

    f = open(outputFile, "w")
    content = ""
    content +="{\n"
    content +="   \"workflow\" :\n"
    content +="   {\n"
    content +="   \"check_types\" : \""+str(check_types)+"\",\n"
    content +="   \"nodes\" : [\n"

    i = 0
    for node in graph.nodes_iter(data=True):
        content +="      {\n"
        content +="       \"start_proc\" : "+str(node[1]['start_proc'])
        content +=",\n       \"nprocs\" : "+str(node[1]['nprocs'])
        content +=",\n       \"func\" : \""+node[1]['func']+"\""
        content +="\n      },\n"

        node[1]['index'] = i
        i += 1

    content = content.rstrip(",\n")
    content += "\n"
    content +="   ],\n"
    content +="\"edges\" : [\n"

    for edge in graph.edges_iter(data=True):
        prod  = graph.node[edge[0]]['index']
        con   = graph.node[edge[1]]['index']


        content +="      {\n"
        content +="       \"start_proc\" : "+str(edge[2]['start_proc'])
        content +=",\n       \"nprocs\" : "+str(edge[2]['nprocs'])
        content +=",\n       \"source\" : "+str(prod)
        content +=",\n       \"target\" : "+str(con)
        content +=",\n       \"prod_dflow_redist\" : \""+edge[2]['prod_dflow_redist']+"\""

        if edge[2]['nprocs'] != 0 :
          content +=",\n       \"dflow_con_redist\" : \""+edge[2]['dflow_con_redist']+"\""
          content +=",\n       \"func\" : \""+edge[2]['func']+"\""
          content +=",\n       \"path\" : \""+edge[2]['path']+"\""

        if "keys" in edge[2]:
          content += ",\n       \"keys\" : "+json.dumps(edge[2]['keys'], sort_keys=True)
        if "prodPort" in edge[2]:
          content += ",\n       \"sourcePort\" : \""+str(edge[2]['prodPort'])+"\""
          content += ",\n       \"targetPort\" : \""+str(edge[2]['conPort'])+"\""
        #if "keys" in edge[2]:
          #content +=",\n       \"keys\" : "+json.dumps(edge[2]['keys'], sort_keys=True)

        content +="\n      },\n"

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
  check_types = 0 means no typechecking; 1 means typechecking at python script only
  2 means typechecking both at python script and at runtime; Only relevant if contracts are used
"""
def processGraph(graph, name, check_types = 1, mpirunPath = "", mpirunOpt = ""):
    check_contracts(graph, check_types)
    processTopology(graph)
    workflowToJson(graph, name+".json", check_types)
    workflowToSh(graph, name+".sh", mpirunOpt = mpirunOpt, mpirunPath = mpirunPath)
