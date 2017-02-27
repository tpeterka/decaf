# converts an nx graph into a workflow data structure and runs the workflow

import imp
import sys
import os
import exceptions
import getopt
import argparse
import json

from collections import defaultdict

import fractions  #Definition of the least common multiple of two numbers; Used for the periodicity
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

  def addContract(self, contract):
    self.contract = contract
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

  def setAny(self, bany): # If bAny is set to true, the runtime considers there is a ContractLink, even if inputs and outputs are empty
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", setting the boolean bAny failed, continuing" % (self.src, self.dest)
    else:
      self.bAny = bany


  def addContractLink(self, cLink):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding a ContractLink is useless, continuing." % (self.src, self.dest)
    self.inputs = cLink.inputs
    self.outputs = cLink.outputs
    self.bAny = cLink.bAny

  def addInputFromDict(self, dict):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Inputs is useless, continuing." % (self.src, self.dest)
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.inputs[key] = val

  def addInput(self, portname, key, typename, period=1):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Inputs is useless, continuing." % (self.src, self.dest)
    self.inputs[key] = [typename, period]


  def addOutputFromDict(self, dict):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Outputs is useless, continuing." % (self.src, self.dest)   
    for key, val in dict.items():
      if len(val) == 1:
        val.append(1)
      self.outputs[key] = val

  def addOutput(self, portname, key, typename, period=1):
    if self.nprocs == 0:
      print "WARNING: There is no proc in the Edge \"%s->%s\", adding Outputs is useless, continuing." % (self.src, self.dest)
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
  """if hasattr(n, 'topology'):
    graph.add_node(n.name, topology=n.topology, func=n.func, cmdline=n.cmdline, inports=n.inports, outports=n.outports)
  else:
    graph.add_node(n.name, nprocs=n.nprocs, start_proc=n.start_proc, func=n.func, cmdline=n.cmdline, inports=n.inports, outports=n.outports)
  """

""" To easily add an Edge to the nx graph """
def addEdge(graph, e):
  graph.add_edge(e.src, e.dest, edge=e)
  """if hasattr(e, 'topology'):
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
# End of addEdge
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

""" Object for contract associated to a link """
class ContractLink(Contract):
  def __init__(self, bany = True, jsonfilename = ""):
    Contract.__init__(self, jsonfilename) 
    self.bAny = bany #If bAny set to True, the fields of contract are required but anything else will be forwarded as well
#End class ContractLink(Contract)


# Checks the contracts between each pair of producer/consumer, taking into account the contracts on links if any
def check_contracts(graph, filter_level):
    if filter_level == Filter_level.NONE:
      return
    # TODO remove all checks of filter_level == NONE in the rest of this function (and function checkWithPorts, checkWithoutPorts also) !!!
    # Well, check if we can remove it...
      
    my_list = [] # To check if an input port is connected to only one output port
    my_dict = defaultdict(set)
    
    for graphEdge in graph.edges_iter(data=True):
        edge = graphEdge[2]["edge"]              # Edge object
        prod = graph.node[graphEdge[0]]["node"]  # Node object for prod
        con = graph.node[graphEdge[1]]["node"]   # Node object for con

        if edge.srcPort != '' and edge.destPort != '' : # If the prod/con of this edge have ports
          (keys, keys_link) = checkWithPorts(edge, prod, con, my_list, filter_level)
          if keys != 0:
            graphEdge[2]['keys'] = keys
          if keys_link != 0:
            graphEdge[2]['keys_link'] = keys_link
            if hasattr(edge, 'attachAny'):
              graphEdge[2]['bAny'] = edge.bAny
        else: #This is the old checking of keys/types with contracts without ports
          if hasattr(prod, 'contract') and hasattr(con, 'contract'):
            (keys, keys_link) = checkWithoutPorts(edge, prod, con, my_dict, filter_level)
            graphEdge[2]['keys'] = keys
            if keys_link != 0 :
              graphEdge[2]['keys_link'] = keys_link
              if hasattr(edge, 'attachAny'):
                graphEdge[2]['bAny'] = edge.bAny
        
    # This for loop is used with the old checking without ports
    for name, set_k in my_dict.items():
      needed = graph.node[name]["node"].contract.inputs
      received = list(set_k)
      complement = [item for item in needed if item not in received]
      if len(complement) != 0:
        s = "ERROR %s does not receive the following:" % name
        for key in complement:
          s+= " %s:%s," %(key, needed[key])
        raise ValueError(s.rstrip(','))
# End of function check_contracts

# Returns the list of fields to be exchanged between the corresponding ports of prod and con in this edge
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
            lcm_val = lcm(Inport[key][1], Outport[key][1])
            intersection_keys[key].append(lcm_val)
            #if lcm_val != Inport[key][1]: # no need to print this warning since the field is just here to be forwarded to the consumer
            #  print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, lcm_val, con_in[key][1])
          if len(intersection_keys) != 0:
            print "WARNING: the link of \"%s.%s->%s\" has no inputs, only fields needed by the Input port will be received" % (edge.src, OutportName, s)
            keys = intersection_keys
          else: # There is nothing to sent by the producer, would block at runtime
            raise ValueError("ERROR: the link of \"%s.%s->%s\" has no inputs and there is no field in common between the Output and Input ports, aborting." % (edge.src, OutportName, s))
    else: # len(edge.inputs) != 0
      if len(Outport) == 0:
        if edge.bAny == True:
          print "WARNING: The link of \"%s.%s->%s\" will accept anything from the Output port." % (edge.src, OutportName, s)
          if len(Inport) == 0:
            edge.attachAny = True
            keys = edge.inputs
          else:
            keys = 0
        else:
          print "WARNING: The Output port of \"%s.%s->%s\" can send anything, only fields needed by the link will be kept, the periodicity may not be correct." % (edge.src, OutportName, s)
          keys = edge.inputs
      else : # len(Outport) != 0
        list_fields = {}
        sk = ""
        for key, val in edge.inputs.items():
          if not key in Outport:
            sk += key + ", "
          else:
            if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]):
              lcm_val = lcm(val[1], Outport[key][1])
              list_fields[key] = [val[0], lcm_val]
              if lcm_val != val[1]:
                print "WARNING: the link of \"%s.%s->%s\" will receive %s with periodicity %s instead of %s." % (edge.src, OutportName, s, key, lcm_val, val[1])
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
                lcm_val = lcm(val[1], Outport[key][1]) # Periodicity for this field is the Least Common Multiple b/w the periodicity of the input and output contracts
                list_fields[key] = [val[0], lcm_val]
                if lcm_val != val[1]:
                  print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, lcm_val, val[1])
              else:
                raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

          if sk != "":
            raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
          keys_link = list_fields
          print "WARNING: The link of \"%s.%s->%s\" has no outputs, all fields needed by the Input port will be forwarded." % (edge.src, OutportName, s)
    else: # len(edge.outputs) != 0
      if len(Inport) == 0:
        if edge.bAny == False:
          keys_link = edge.outputs
        else: # Need to attach keys of output link and of output Port
          print "WARNING: The link of \"%s.%s->%s\" will forward anything to the Input port." % (edge.src, OutportName, s)
          if len(Outport) == 0: # If there are no contracts on Nodes but only contract on the Link
            edge.attachAny = True
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
              lcm_val = lcm(val[1], edge.outputs[key][1])
              list_fields[key] = [val[0], lcm_val]
              if lcm_val != val[1]:
                print "WARNING: the Input port of \"%s.%s->%s\" will receive %s with periodicity %s instead of %s." % (edge.src, OutportName, s, key, lcm_val, val[1])
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
            print "WARNING: The link of \"%s.%s->%s\" will forward anything from the Output port, cannot guarantee the fields needed by the Input port will be present." % (edge.src, OutportName, s)
            keys_link = list_fields
          else: # Complete with the keys of the Output port
            sk = ""
            for key, val in tmp.items():
              if not key in Outport:
                sk += key + ", "
              else:
                if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]):
                  lcm_val = lcm(val[1], Outport[key][1])
                  list_fields[key] = [val[0], lcm_val]
                  if lcm_val != val[1]:
                    print "WARNING: the Input port of \"%s.%s->%s\" will receive %s with periodicity %s instead of %s." % (edge.src, OutportName, s, key, lcm_val, val[1])
                else:
                  raise ValueError("ERROR: the types of \"%s\" does not match for the Output port and the Input port of \"%s.%s->%s\", aborting." % (key, edge.src, OutportName, s))
            if sk != "":
              raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port of \"%s.%s->%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName, s))
            keys_link = list_fields

    return (keys, keys_link)
  # End of if hasattr(edge,'bAny')
  else:
    if len(Inport) == 0: # The Input port accepts anything, send only keys in the output port contract
      if len(Outport) == 0:
        print "WARNING: The port \"%s\" will accept everything sent by \"%s.%s\"." % (s, edge.src, OutportName)
        return (0,0)
      else:
        return (Outport, 0)

    if len(Outport) == 0: # The Output port can send anything, keep only the list of fields needed by the consumer
      print "WARNING: The Output port \"%s.%s\" can send anything, only fields needed by the Input port \"%s\" will be kept, the periodicity may not be correct." % (edge.src, OutportName, s)
      return (Inport, 0)

    else: # Checks if the list of fields of the consumer is a subset of the list of fields from the producer
      list_fields = {}
      sk = ""
      for key, val in Inport.items():
        if not key in Outport : # The key is not sent by the producer
          sk+= key + ", "
        else:
          if (filter_level == Filter_level.NONE) or (val[0] == Outport[key][0]) : # The types match, add the field to the list and update the periodicity
            lcm_val = lcm(val[1], Outport[key][1]) # Periodicity for this field is the Least Common Multiple b/w the periodicity of the input and output contracts
            list_fields[key] = [val[0], lcm_val]
            if lcm_val != val[1]:
              print "WARNING: %s will receive %s with periodicity %s instead of %s." % (edge.dest, key, lcm_val, val[1])
          else:
            raise ValueError("ERROR: the types of \"%s\" does not match for the ports \"%s.%s\" and \"%s.%s\", aborting." % (key, edge.src, OutportName, edge.dest, InportName))

      if sk != "":
        raise ValueError("ERROR: the fields \"%s\" are not sent in the Output port \"%s.%s\", aborting." % (sk.rstrip(", "), edge.src, OutportName))
    return (list_fields, 0) # This is the dict of fields to be exchanged in this edge; A field is an entry of a dict: name:[type, period]
# end of checkWithPorts

def checkWithoutPorts(edge, prod, con, my_dict, filter_level):
    if prod.contract.outputs == {}:
        raise ValueError("ERROR while checking contracts: %s has no outputs, aborting." % prod.name)
    else:
        prod_out = prod.contract.outputs

    if con.contract.inputs == {}:
        raise ValueError("ERROR while checking contracts: %s has no inputs, aborting." % con.name)
    else:
        con_in = con.contract.inputs

    if hasattr(edge, 'bAny'):
      if edge.inputs == {}:
        raise ValueError("ERROR while checking contracts: the link %s->%s has no inputs, aborting." % (prod.name, con.name))
      if edge.outputs == {}:
        raise ValueError("ERROR while checking contracts: the link %s->%s has no outputs, aborting." % (prod.name, con.name))
      keys = {}
      keys_link = {}
      # Match contract prod with link input
      list_fields = {}
      sk = ""
      for key, val in edge.inputs.items(): # Check if edge.inputs included in prod_out
        if not key in prod_out:
          sk += key + ", "
        else:
          if (filter_level == Filter_level.NONE) or (val[0] == prod_out[key][0]):
            lcm_val = lcm(val[1], prod_out[key][1])
            list_fields[key] = [val[0], lcm_val]
            if lcm_val != val[1]:
              print "WARNING: the link of \"%s->%s\" will receive %s with periodicity %s instead of %s." % (edge.src, edge.dest, key, lcm_val, val[1])
          else:
            raise ValueError("ERROR: the types of \"%s\" does not match for the producer and the link of \"%s->%s\", aborting." % (key, edge.src, edge.dest))
      if sk != "":
        raise ValueError("ERROR: the fields \"%s\" are not sent to the link of \"%s->%s\", aborting." % (sk.rstrip(", "), edge.src, edge.dest))
      keys = list_fields
      if edge.bAny == True: # We complete with the other keys in prod_out if they are in con_in and of same type
        for key, val in prod_out.items():
          if (not key in keys) and (key in con_in) and (prod_out[key][0] == con_in[key][0]): #TODO verify this
            keys[key] = val

      # Match link output with contract Con
      intersection_keys = {key:[con_in[key][0]] for key in con_in.keys() if (key in edge.outputs) and ( (filter_level == Filter_level.NONE) or (con_in[key][0] == edge.outputs[key][0]) ) }
      for key in intersection_keys.keys():
        my_dict[con.name].add(key)
        lcm_val = lcm(con_in[key][1], edge.outputs[key][1])
        intersection_keys[key].append(lcm_val)
        if lcm_val != con_in[key][1]:
          print "WARNING: %s will receive %s with periodicity %s instead of %s." % (con.name, key, lcm_val, con_in[key][1])
      if len(intersection_keys) == 0:
        raise ValueError("ERROR: the intersection of fields between the link and consumer of \"%s->%s\" is empty, aborting.", prod.name, con.name)

      keys_link = intersection_keys
      if edge.bAny == True: # Complete keys_link with the keys that are both in Prod and Con contracts
        intersect = {key:[con_in[key][0]] for key in con_in.keys() if (key in prod_out) and ( (filter_level == Filter_level.NONE) or (con_in[key][0] == prod_out[key][0]) ) }
        for key in intersect.keys():
          my_dict[con.name].add(key)
          lcm_val = lcm(con_in[key][1], prod_out[key][1])
          intersect[key].append(lcm_val)
          if lcm_val != con_in[key][1]:
            print "WARNING: %s will receive %s with periodicity %s instead of %s." % (con.name, key, lcm_val, con_in[key][1])
        # Add keys if in intersection but not in keys_link
        for key, val in intersect.items():
          if not key in keys_link:
            keys_link[key] = val

      return (keys, keys_link)
    else: # no attr 'bAny'
      # We add for each edge the list of pairs key/type which is the intersection of the two contracts
      intersection_keys = {key:[con_in[key][0]] for key in con_in.keys() if (key in prod_out) and ( (filter_level == Filter_level.NONE) or (con_in[key][0] == prod_out[key][0]) ) }
      for key in intersection_keys.keys():
          my_dict[con.name].add(key)
          lcm_val = lcm(con_in[key][1], prod_out[key][1])
          intersection_keys[key].append(lcm_val)
          if lcm_val != con_in[key][1]:
            print "WARNING: %s will receive %s with periodicity %s instead of %s." % (con.name, key, lcm_val, con_in[key][1])

      if len(intersection_keys) == 0:
          raise ValueError("ERROR: the intersection of fields between %s and %s is empty, aborting." % (prod.name, con.name))
      return (intersection_keys, 0)


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
    return Topology("global", args.nprocs, hostfilename = args.hostfile)

""" Not used anymore if we are using Node and Edge objects
def processTopology(graph):
    # Check in all nodes and edge if a topology is present.
    #    If yes, fill the fields start_proc and nprocs 
    
    for node in graph.nodes_iter(data=True):
        if 'topology' in node[1]:
            topo = node[1]['topology']
            node[1]['start_proc'] = topo.offsetRank
            node[1]['nprocs'] = topo.nProcs
            # Clear the graph
            del node[1]['topology'] #TODO is it really useful to remove this ? No real memory consumption


    for edge in graph.edges_iter(data=True):
        if 'topology' in edge[2]:
            topo = edge[2]['topology']
            edge[2]['start_proc'] = topo.offsetRank
            edge[2]['nprocs'] = topo.nProcs
            # Clear the graph
            del edge[2]['topology'] #TODO is it really useful to remove this ? No real memory consumption
"""


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
                                          "prod_dflow_redist" : edge.prod_dflow_redist})

        if edge.nprocs != 0:
          data["workflow"]["edges"][i].update({"func" : edge.func,
                                          "dflow_con_redist" : edge.dflow_con_redist,
                                          "path" : edge.path})
          # Used for ContractLink
          if "keys_link" in graphEdge[2]:
            data["workflow"]["edges"][i]["keys_link"] = graphEdge[2]['keys_link']
            if "bAny" in graphEdge[2]:
              data["workflow"]["edges"][i]["bAny"] = graphEdge[2]['bAny']
        
        # TODO put stream and others in the Edge object?
        if 'stream' in graphEdge[2]:
            data["workflow"]["edges"][i]["stream"] = graphEdge[2]['stream']
            data["workflow"]["edges"][i]["frame_policy"] = graphEdge[2]['frame_policy']
            data["workflow"]["edges"][i]["storage_types"] = graphEdge[2]['storage_types']
            data["workflow"]["edges"][i]["max_storage_sizes"] = graphEdge[2]['max_storage_sizes']
        
        # Used for Contract
        if "keys" in graphEdge[2]:
          data["workflow"]["edges"][i]["keys"] = graphEdge[2]['keys']

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
def updateLinkStream(graph, prod, con, stream, frame_policy, storage_types, max_storage_sizes):
    graph.edge[prod][con]['stream'] = stream
    graph.edge[prod][con]['frame_policy'] = frame_policy
    graph.edge[prod][con]['storage_types'] = storage_types
    graph.edge[prod][con]['max_storage_sizes'] = max_storage_sizes

# Shortcut function to define a sequential streaming dataflow
def addSeqStream(graph, prod, con, buffers = ["mainmem"], max_buffer_sizes = [10]):
    updateLinkStream(graph, prod, con, 'double', 'seq', buffers, max_buffer_sizes)

# Shortcut function to define a stream sending the most recent frame to the consumer
def addMostRecentStream(graph, prod, con, buffers = ["mainmem"], max_buffer_sizes = [10]):
    updateLinkStream(graph, prod, con, 'single', 'recent', buffers, max_buffer_sizes)


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

        if exe.nprocs == 0:
          print 'ERROR: a node can not have 0 MPI rank.'
          exit()

        mpirunCommand += "-np "+str(exe.nprocs)
        mpirunCommand += " "+str(exe.cmdline)+" : "
        currentRank += exe.nprocs

      if type == 'edge':

        if exe.nprocs != 0:
          mpirunCommand += "-np "+str(exe.nprocs)
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

""" Process the graph and generate the necessary files
"""
def processGraph(graph, name, filter_level = Filter_level.NONE, mpirunPath = "", mpirunOpt = ""):
    check_contracts(graph, filter_level)
    #processTopology(graph) Not used anymore if we are using Node and Edge objects in the graph
    workflowToJson(graph, name+".json", filter_level)
    workflowToSh(graph, name+".sh", mpirunOpt = mpirunOpt, mpirunPath = mpirunPath)
