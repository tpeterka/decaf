This document uses the [Markdown](http://daringfireball.net/projects/markdown/) syntax.

# Decaf dependencies

- C++11
- MPI-3 (currently MPI is the only supported transport layer)
- Boost
- Optionally, if you want the python version of the examples to be built,
    - python
    - pybind11
    - networkx

# Building decaf:

Decaf is built using CMake. Assuming that you created a build directory one level below the decaf home directory, then:
```
cd build
```
Configure the build using cmake with this sample command line. We recommend saving this into a script and running the script:

```
cmake .. \
-DCMAKE_INSTALL_PREFIX=/path/to/decaf/install \
-Dtransport_mpi=on \
```

Currently, the available options are:

- transport_mpi      "Build Decaf with MPI transport layer"         ON/OFF, default ON

Then, make, install, and set environment variables:
```
make
make install
export DECAF_PREFIX=/path/to/decaf/install
export DYLD_LIBRARY_PATH=/path/to/decaf/install/lib:$DYLD_LIBRARY_PATH
```
(The syntax above is for Bash and Mac OSX; other shells and unixes are similar; eg., the dynamic library path variable is ```LD_LIBRARY_PATH on Linux```. You may consider setting the environment variables in .bashrc or .profile)

# Building your project with decaf:

There are numerous examples in the [examples directory](https://bitbucket.org/tpeterka1/decaf/raw/master/examples) of how to write your source file(s). Briefly, you need to provide the following:

- Functions for all producer and consumer nodes and dataflow links in your workflow. The signature of producer and consumer nodes is up to the user; dataflow links have the following function signature:
```{.cpp}
extern "C"
{
  int dflow(void* args,                       // custom arguments to the callback
           Dataflow* dataflow,                // this dataflow
           pConstructData in_data)            // input data on this dataflow
  {
  }
}
```

# Wrapping your project in python

The workflow can be hand-coded in C++, but the python instructions below describe an easier way to define the network. The python program creates a JSON configuration file that decaf reads.

A short python script (eg. example.py) can be used to set workflow parameters and run the workflow.
```{python}
# a small 2-node example, just a producer and consumer

# --- include the following 4 lines each time ---

import networkx as nx
import os
import imp
wf = imp.load_source('workflow', os.environ['DECAF_PREFIX'] + '/python/workflow.py')

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

w = nx.DiGraph()
w.add_node("prod", start_proc=0, nprocs=4, func='prod')
w.add_node("con",  start_proc=6, nprocs=2, func='con')
w.add_edge("prod", "con", start_proc=4, nprocs=2, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count')

# --- convert the nx graph into a workflow data structure and run the workflow ---

wf.workflowToJson(w, mod_path, "linear2.json")
```

# Learning more about decaf:

See the documents [here](https://bitbucket.org/tpeterka1/decaf/wiki/public-docs/public-docs.md).

