from flowvrapp import *

model = "/home/matthieu/INRIA/molecules/FEPA/holo314.tpr"

decafmodule = Module("decaf", cmdline = "mpirun -n 4 mdrun_mpi_4.5.5_decaf -s "+model+" : -n 2 dflow_gromacs : -n 2 treatment fepa : -n 1 dflow_gromacs : -n 1 targetmanager_flowvr fepa 5.0 0.5 : -n 1 dflow_gromacs")
outport = decafmodule.addPort("outPos")

app.generate_xml("decaf")
