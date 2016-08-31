from flowvrapp import *
from filters import *

modelTpr = "/home/matthieu/INRIA/molecules/FEPA/holo314.tpr"
modelPdb = "/home/matthieu/INRIA/molecules/FEPA/holo314.pdb"
modelNc  = "/home/matthieu/INRIA/molecules/FEPA/h_chaincolor_P1P2.nc"

class RenduSimple(Component):

  def __init__(self, prefix, hosts):
    Component.__init__(self)

    #rendu = Module(prefix, cmdline = "xterm -hold -e valgrind RenduOpenGLDecaf", host = hosts)
    rendu = Module(prefix, cmdline = "RenduOpenGLDecaf", host = hosts)
    rendu.run.options += "-x DISPLAY "
    rendu.addPort("positions", direction = "in")
    rendu.addPort("idatomselect", direction = "in")
    rendu.addPort("selectionmode", direction = "in")
    rendu.addPort("selectable",direction = "in")
    rendu.addPort("bonds", direction = "in")
    rendu.addPort("atoms", direction = "in")
    rendu.addPort("bbox",direction = "in")
    rendu.addPort("force", direction = "in")
    rendu.addPort("shaders", direction = "in")
    rendu.addPort("Avatar",direction = "in")
    rendu.addPort("Camera",direction = "in")
    rendu.addPort("Activations",direction = "in")
    #rendu.addPort("TroisDButton",direction = "in")
    rendu.addPort("TroisDAnalog",direction = "in")
    rendu.addPort("PhantomCalibration",direction = "in")
    rendu.addPort("Params3D",direction = "in")
    rendu.addPort("Targets",direction = "in")
    rendu.addPort("SelectionPos",direction = "in")
    rendu.addPort("InBBoxBase",direction = "in")
    rendu.addPort("InBBoxRedistribute",direction = "in")
    rendu.addPort("InGridGlobal",direction = "in")
    rendu.addPort("InGridActive",direction = "in")
    rendu.addPort("InMesh",direction = "in")
    rendu.addPort("InBestPos", direction="in")
    rendu.addPort("InMatrix", direction="in")
    rendu.addPort("OutAvatar",direction = "out")
    rendu.addPort("OutMatrix", direction = "out")
    rendu.addPort("OutGDeselector",direction = "out")

    ### expose input and output ports

    self.ports = rendu.ports

class NetCDFMolRepReader(Component):

  def __init__(self, prefix, hosts, fileNc):
    Component.__init__(self)

    netcdf = Module(prefix , cmdline = "NetCDFMolRepReaderDecaf " + fileNc, host = hosts)
    netcdf.addPort("outSphere", direction = "out")
    netcdf.addPort("outCylinder", direction = "out")
    netcdf.addPort("outCenter", direction = "out")
    netcdf.addPort("outPosition", direction = "out")
    netcdf.addPort("outBBox", direction = "out")

    ### expose input and output ports
    self.ports = netcdf.ports

class DoubleBufferedFilterIt(flowvrapp.Filter):

  def __init__(self, name, host = ''):
    flowvrapp.Filter.__init__(self, name, run = 'flowvr.plugins.DoubleBufferedFilterIt', host = host)
    self.addPort('in', direction = 'in')
    self.addPort('out', direction = 'out')
    self.addPort('order', direction = 'in', messagetype = 'stamps')

#Module declaration
decafmodule = Module("decaf", cmdline = "mpirun -n 4 mdrun_mpi_4.5.5_decaf -v -s "+modelTpr+" : -n 2 dflow_gromacs : -n 2 treatment fepa : -n 1 dflow_gromacs : -n 1 targetmanager_flowvr fepa 2.0 0.5 : -n 1 dflow_gromacs")
decafmodule.addPort("outPos")
decafmodule.addPort("outTargets")
decafmodule.addPort("outSelection")

#Visualization modules
NC = NetCDFMolRepReader("NetCDFMolRepReader", "localhost", modelNc)
gPositions = Greedy("gPositions",DoubleBufferedFilterIt, host="localhost")
gTargets = Greedy("gTargets",DoubleBufferedFilterIt, host="localhost")
gSelect = Greedy("gSelect",DoubleBufferedFilterIt, host="localhost")
renduopengl = RenduSimple("RenduSimple", "localhost") #Ajouter le parametre fpsLimit

#Decaf to renderer
decafmodule.getPort("outPos").link(gPositions.getPort("in"))
gPositions.getPort("out").link(renduopengl.getPort("positions"))
renduopengl.getPort("endIt").link(gPositions.getPort("sync"))

decafmodule.getPort("outTargets").link(gTargets.getPort("in"))
gTargets.getPort("out").link(renduopengl.getPort("Targets"))
renduopengl.getPort("endIt").link(gTargets.getPort("sync"))

decafmodule.getPort("outSelection").link(gSelect.getPort("in"))
gSelect.getPort("out").link(renduopengl.getPort("SelectionPos"))
renduopengl.getPort("endIt").link(gSelect.getPort("sync"))

#NC to renderer
NC.getPort("outSphere").link(renduopengl.getPort("atoms"))
NC.getPort("outCylinder").link(renduopengl.getPort("bonds"))
NC.getPort("outBBox").link(renduopengl.getPort("bbox"))

app.generate_xml("decaf")
