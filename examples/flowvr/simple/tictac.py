from flowvrapp import *

putmodule = Module("put", cmdline = "xterm -hold -e gdb put")
outport = putmodule.addPort("text", direction = "out")

getmodule = Module("get", cmdline = "xterm -hold -e gdb get")
inport = getmodule.addPort("text", direction = "in")

outport.link(inport)

app.generate_xml("tictac")
