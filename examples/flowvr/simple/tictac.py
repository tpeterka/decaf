from flowvrapp import *

putmodule = Module("put", cmdline = "put")
outport = putmodule.addPort("text", direction = "out")

getmodule = Module("get", cmdline = "get")
inport = getmodule.addPort("text", direction = "in")

outport.link(inport)

app.generate_xml("tictac")
