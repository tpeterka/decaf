# Decaf

Decaf is a dataflow system for the parallel communication of coupled tasks in an
HPC workflow. The dataflow can perform arbitrary data transformations ranging
from simply forwarding data to complex data redistribution. Decaf does this by
allowing the user to allocate resources and execute custom code in the dataflow.
All communication through the dataflow is efficient parallel message passing
over MPI. The runtime for calling tasks is entirely message-driven; Decaf
executes a task when all messages for the task have been received. Such a
message-driven runtime allows cyclic task dependencies in the workflow graph,
for example, to enact computational steering based on the result of downstream
tasks. Decaf includes a simple Python API for describing the workflow graph.
This allows Decaf to stand alone as a complete workflow system, but Decaf
can also be used as the dataflow layer by one or more other workflow systems
to form a heterogeneous task-based computing environment.

# Decaf dependencies

- C++11
- MPI-3
- Boost 1.59 or higher
- Python 3.0 or higher
- Networkx 2.0
- CMake 3.0 or higher
- CCI 2.1 or higher (optional)



# Building Decaf

Retrieve the sources of Decaf (in the current directory, e.g.):
```
git clone https://github.com/tpeterka/decaf.git .
```

Decaf is built using CMake. Assuming that you created a build directory, then:
```
cd path/to/decaf/build
cmake /path/to/decaf/source \
-DCMAKE_INSTALL_PREFIX=/path/to/decaf/install \
-Dtransport_mpi=on \
```

Then, make, install, and set environment variables:
```
make
make install
export DECAF_PREFIX=/path/to/decaf/install
export LD_LIBRARY_PATH=/path/to/decaf/install/lib:$LD_LIBRARY_PATH
```
(The syntax above is for Bash and Linux; other shells and unixes are similar;
eg., the dynamic library path variable is ```DYLD_LIBRARY_PATH on Mac OSX```.
You may consider setting the environment variables in .bashrc or .profile)

# Running examples

Decaf provides several examples of simple workflows. Assuming Decaf was installed following the previous instructions, run the following commands:
```
cd /path/to/Decaf/install/examples/direct
python3 linear-2nodes.py
./linear2.sh

cd /path/to/Decaf/install/examples/direct
python3 cycle-4nodes.py
./cycle.sh


```

# Describing a workflow in Python

The workflow can be hand-coded in C++, but the Python instructions below
describe an easier way to define the network. The Python program creates a JSON
configuration file that Decaf reads.

A short Python script (eg. example.py) can be used to set workflow parameters and run the workflow.
```{python}
# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/decaf.py')

# --- set your options here ---

# path to .so module for dataflow callback functions
mod_path = os.environ['DECAF_PREFIX'] + '/examples/direct/mod_linear_2nodes.so'

# define workflow graph
# 2-node workflow
#
#    prod (4 procs) -> con (2 procs)
#
#  entire workflow takes 8 procs (2 dataflow procs between producer and consumer)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

# --- Graph definition ---
prod = wf.Node("prod", start_proc=0, nprocs=4, func='prod', cmdline='./linear_2nodes')
outPort = prod.addOutputPort("out")

con = wf.Node("con", start_proc=6, nprocs=2, func='con', cmdline='./linear_2nodes')
inPort = con.addInputPort("in")

link = wf.Edge(prod.getOutputPort("out"), con.getInputPort("in"), start_proc=4, nprocs=2, func='dflow',
        path=mod_path, prod_dflow_redist='count', dflow_con_redist='count', cmdline='./linear_2nodes')

# --- convert the nx graph into a workflow data structure and run the workflow ---
wf.processGraph("linear2")
```
# Licensing

Decaf is open source software under a [BSD-3 license](https://github.com/tpeterka/decaf/blob/master/COPYING).

# Learning more about Decaf

See the documents [here](https://tpeterka.github.io/decaf/).

