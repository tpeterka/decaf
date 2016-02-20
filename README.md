This document uses the [Markdown](http://daringfireball.net/projects/markdown/) syntax.

# Decaf dependencies

- C++11
- MPI-3 (currently MPI is the only supported transport layer)
- Boost
- Optionally, if you want the python version of the examples to be built,
    - python
    - cython
    - networkx

# Building decaf:

Decaf is built using CMake. Assuming that you created a build directory one level below the decaf home directory, then:
```
cd build
```
Configure the build using cmake with this sample command line. We recommend saving this into a script and running the script:

```
cmake .. \
-DCMAKE_CXX_COMPILER=mpicxx \
-Ddebug=on \
-Doptimize=off \
-Dtransport_mpi=on \
```

Currently, the available options are:

- debug              "Build Decaf with debugging on"                OFF/ON, default OFF
- optimize           "Build Decaf with optimization"                OFF/ON, default OFF
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

To use decaf in C++ source code, Simply,

```{.cpp}
#include <decaf/decaf.hpp>
```
All of decaf is contained in header files; there is no need to link with a library. There are numerous examples in the [examples directory](https://bitbucket.org/tpeterka1/decaf/raw/master/examples) of how to write your source file(s). Briefly, you need to provide the following:

- Callback functions for all producer and consumer nodes and dataflow links in your workflow. Producer and consumer nodes have the following function signature:
```{.cpp}
int func(void* args,                                  // custom arguments to the callback
         vector<Dataflow*>* out_dataflows,            // all outbound dataflows
         vector<shared_ptr<ConstructData> >* in_data) // input data in order of inbound dataflows
```

Dataflow links have the following function signature:
```{.cpp}
int func(void* args,                        // custom arguments to the callback
         Dataflow* dataflow,                // this dataflow
         shared_ptr<ConstructData> in_data) // input data on this dataflow
```
The return value of the callback functions (both producer/consumer nodes and dataflow links) tells decaf whether to continue to call this function in the future (`return 0;`), or whether this node or link is done and should not be called anymore (`return 1;`). Typically, one node or link in the workflow will initiate the shutdown of the workflow by sending a special quit message in its outbound link(s). That function should return 1 after doing so, to prevent decaf from calling it again. Decaf will propagate the quit message to the rest of the workflow, shutting down the remaining nodes and links automatically as they receive a quit message. However, it is the responsibility of the originator of the shutdown to terminate itself by returning 1.

The definitions of the callbacks must be wrapped in the `extern "C"` calling signature. Inside of the C calling bindings, the functions can be written in C++. Because of the C calling conventions, names in the same file must be unique. The callbacks must be built into a shared object (dynamically loaded library or plugin).
```{.cpp}
extern "C"
{
  int producer(...)
  {
    ...
  }
  int consumer(...)
  {
    ...
  }
  int dataflow(...)
  {
    ...
  }
}
```

- A workflow takes the following form:
```{.cpp}
struct Workflow          // an entire workflow
{
  vector<WorkflowNode> nodes; // all the workflow nodes
  vector<WorkflowLink> links; // all the workflow links
};
```
Where a workflow node is defined as:
```{.cpp}
struct WorkflowNode      // a producer or consumer node
{
  vector<int> out_links; // indices of outgoing links
  vector<int> in_links;  // indices of incoming links
  int start_proc;        // starting proc rank in world communicator for this producer or consumer
  int nprocs;            // number of processes for this producer or consumer
  string func;           // name of callback function
  void* args;            // custom callback arguments
  string path;           // path to callback function module
};
```
And a worfklow link is the following:
```{.cpp}
struct WorkflowLink         // a dataflow link
{
  int prod;                 // index in vector of all workflow nodes of producer
  int con;                  // index in vector of all workflow nodes of consumer
  int start_proc;           // starting process rank in world communicator for the dataflow
  int nprocs;               // number of processes in the dataflow
  string func;              // name of callback function
  void* args;               // custom callback arguments
  string path;              // path to callback function module
  string prod_dflow_redist; // redistribution component between producer and dataflow
  string dflow_con_redist;  // redistribution component between dataflow and consumer
};
```
The workflow can be hand-coded, but the python instructions below describe an easier way to define the network.

- A `run()` function for the workflow, that takes the workflow as an input argument, along with any other arguments, such as the number of producer and consumer time steps.

```{.cpp}
void run(Workflow& workflow,  // workflow
         vector<int> sources) // indices of workflow nodes that start the workflow execution
{

  // create any arguments needed to pass to the callbacks
  // add the args to the workflow nodes and links

  MPI_Init(NULL, NULL);

  // create and run decaf
  Decaf* decaf = new Decaf(MPI_COMM_WORLD, workflow);
  decaf->run(&pipeliner, &checker, sources);

  // cleanup
  // delete callback args
  delete decaf;
  MPI_Finalize();
}
```
`pipeliner` and `checker` are placeholders for future functionality. For now, simply define stubs for them as follows:

````{.cpp}
void pipeliner(Dataflow* decaf){}
void checker(Dataflow* decaf) {}
````
- Optionally, a `main()` function can be used to define the workflow. For example, in the decaf examples, `main()` is used to hard-code test values into the workflow. A more useful alternative is described below: define your workflow in python. If the project is wrapped in python, the `run()` function described above is the entry point to the C++ code; `main()` is not called by python.

# Wrapping your project in python

A short python script (eg. driver.py) can be used to set workflow parameters, import the precompiled module for the project, and run it. An example script [examples/lammps/python/driver-4-nodes.py](https://bitbucket.org/tpeterka1/decaf/raw/master/examples/lammps/python/driver-4nodes.py) appears below:
```{python}
import networkx as nx

# --- set your options here ---

# path to .so module
path = '/Users/tpeterka/software/decaf/install/examples/lammps/python/libpy_lammps.so'

# define workflow graph
# 4-node workflow
#
#          print1 (1 proc)
#        /
#    lammps (4 procs)
#        \
#          print2 (1 proc) - print3 (1 proc)
#
#  entire workflow takes 10 procs (1 dataflow proc between each producer consumer pair)
#  dataflow can be overlapped, but currently all disjoint procs (simplest case)

w = nx.DiGraph()

# example of 4 nodes and 3 edges (single source)
# this is the example diagrammed above, and for which driver.pyx is made
w.add_node("lammps", start_proc=0, nprocs=4, prod_func='lammps'     , con_func=''          )
w.add_node("print1", start_proc=5, nprocs=1, prod_func= ''          , con_func='print'     )
w.add_node("print2", start_proc=7, nprocs=1, prod_func='print2_prod', con_func='print2_con')
w.add_node("print3", start_proc=9, nprocs=1, prod_func=''           , con_func='print'     )
w.add_edge("lammps", "print1", start_proc=4, nprocs=1, dflow_func='dflow'                  )
w.add_edge("lammps", "print2", start_proc=6, nprocs=1, dflow_func='dflow'                  )
w.add_edge("print2", "print3", start_proc=8, nprocs=1, dflow_func='dflow'                  )

# sources
sources = [0]

# lammps input file
infile = "/Users/tpeterka/software/decaf/examples/lammps/in.melt"

# --- do not edit below this point --

# call driver

import imp
driver = imp.load_dynamic('driver', path)
driver.pyrun(w, sources, infile)
```
[More information on the steps to generate a python interface for a project](https://bitbucket.org/tpeterka1/decaf/raw/master/doc/python.md)

# Executing the python-wrapped project

```
mpiexec -n <num_procs> python driver.py
```

# Documentation

See the documentation in the [doc directory](https://bitbucket.org/tpeterka1/decaf/raw/master/doc).

# Learning more about decaf:

See the documents [here](https://bitbucket.org/tpeterka1/decaf/wiki/public-docs/public-docs.md).

