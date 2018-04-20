This document uses the [Markdown](http://daringfireball.net/projects/markdown/) syntax.

# Decaf dependencies

- C++11
- MPI-3
- Boost 1.59 or higher
- Python 2.7
- Networkx 2.0
- CMake 3.0 or higher
- CCI 2.1 or higher (optional)

The following is an example of commands to install the various dependencies of
Decaf on a Linux system. In this example, the dependencies installed from source are built in the
folder ```$HOME/software``` and installed in ```$HOME/software/install```. The
example installs OpenMPI but any MPI package supporting MPI 3.0 can be used.

```
sudo apt-get install git
sudo apt-get install cmake
sudo apt-get install python-pip

# Minimum OpenMPI version: 1.8
sudo apt-get install openmpi-common
sudo apt-get install openmpi-bin
sudo apt-get install libopenmpi-dev

cd $HOME
mkdir software
cd software
mkdir install

export PATH=$HOME/software/install/include:$HOME/software/install/bin:$PATH
export LD_LIBRARY_PATH=$HOME/software/install/lib:$LD_LIBRARY_PATH

# Boost
cd $HOME/software
wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz
tar -xf boost_1_64_0.tar.gz
cd boost_1_64_0
./bootstrap.sh --prefix=$HOME/software/install/
./b2 install

# NetworkX
pip install networkx

# Optional: CCI
cd $HOME/software
wget http://cci-forum.com/wp-content/uploads/2017/05/cci-2.1.tar.gz
tar -xf cci-2.1.tar.gz
cd cci-2.1
./configure --prefix=$HOME/software/install
make -j4 install
```

# Building Decaf:

Retrieve the sources of Decaf (in the current directory, e.g.):
```
git clone https://bitbucket.org/tpeterka1/decaf.git .
```

Decaf is built using CMake. Assuming that you created a build directory, then:
```
cd path/to/decaf/build
```

Currently, the available options are:

- transport_mpi      "Build Decaf with MPI transport layer"          ON/OFF, default ON
- transport_cci      "Build Decaf with CCI transport layer"          ON/OFF, default OFF
- build_bredala      "Build Bredala libraries and examples"          ON/OFF, default ON
- build_manala       "Build Manala libraries and examples"           ON/OFF, default ON
- build_decaf        "Build the Decaf workflow system"               ON/OFF, default ON
- build_tests        "Build the tests examples"                      ON/OFF, default ON

```transport_mpi``` and ```transport_cci``` are the two transport methods
currently supported by Decaf. At least one transport method must be turned on,
but more than one are allowed if different parts of the workflow graph use
different methods.
The Decaf project is composed of three parts: Bredala, a data model and
transport library, Manala, a library managing the flow of messages between two
tasks, and Decaf, a runtime coordinating tasks within a workflow. It is possible
to compile Bredala alone, Manala with Bredala, or Decaf with Manala and Bredala. 

The following command builds Decaf with the MPI transport method. We recommend saving this into a script and running the script:

```
cmake /path/to/decaf/source \
-DCMAKE_INSTALL_PREFIX=/path/to/decaf/install \
-Dtransport_mpi=on \
```

The following command builds Decaf with both CCI and MPI transport methods:
```
cmake .. \
-DCMAKE_INSTALL_PREFIX=/path/to/decaf/install \
-Dtransport_mpi=on \
-Dtransport_cci=on \
-DCCI_PREFIX:PATH=/path/to/cci/install # If CCI is not installed in a default path
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

# Testing the installation on a local machine

Decaf provides several examples of simple workflows. Assuming Decaf was installed following the previous instructions, run the following commands:
```
# Testing examples
# All the examples should complete
cd /path/to/Decaf/install/examples/direct
python linear-2nodes.py
./linear2.sh

cd /path/to/Decaf/install/examples/direct
python linear-3nodes.py
./linear3.sh

cd /path/to/Decaf/install/examples/direct
python cycle-4nodes.py
./cycle.sh

# Testing CCI transport method
# Available only if Decaf was compiled with transport_cci=on
python linear-2nodes-cci.py
./linear2.sh

```
# Testing the installation on a distributed machine

Certain Python scripts from the examples take extra arguments to specify the
number of cores and their respective hosts. The following gives an example to
use the linear-2nodes examples in a distributed context:

```
# The linear-2nodes requires 8 procs (4 prod, 2 link, 2 cons)
cd /path/to/Decaf/install/examples/direct
echo -e "node1\nnode1\nnode1\nnode1\nnode2\nnode2\nnode3\nnode3" > hostfile.txt
python linear-2nodes-topo1.py --np 8 --hostfile hostfile.txt
./linear2.sh
```


# Using Decaf in your project:

There are numerous examples in the
[examples directory](https://bitbucket.org/tpeterka1/decaf/raw/master/examples) of how to
write your source file(s). Briefly, you need to provide the following:

- Functions for all producer and consumer nodes and dataflow links in your
  workflow. The signature of producer and consumer nodes is up to the user;
  dataflow links have the following function signature:
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

# Wrapping your project in Python

The workflow can be hand-coded in C++, but the python instructions below
describe an easier way to define the network. The Python program creates a JSON
configuration file that Decaf reads.

A short Python script (eg. example.py) can be used to set workflow parameters and run the workflow.
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
w.add_node("prod", start_proc=0, nprocs=4, func='prod', cmdline='./linear_2nodes')
w.add_node("con",  start_proc=6, nprocs=2, func='con', cmdline='./linear_2nodes')
w.add_edge("prod", "con", start_proc=4, nprocs=2, func='dflow', path=mod_path,
           prod_dflow_redist='count', dflow_con_redist='count', cmdline='./linear_2nodes')

# --- convert the nx graph into a workflow data structure and run the workflow ---

wf.processGraph(w, "linear2")
```

# Learning more about decaf:

See the documents [here](https://bitbucket.org/tpeterka1/decaf/wiki/public-docs/public-docs.md).

