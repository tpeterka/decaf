This document uses the [Markdown](http://daringfireball.net/projects/markdown/) syntax.

# Decaf dependencies

- C++11
- MPI-3 (currently MPI is the only supported transport layer)
- Boost 1.59 or higher
- Python 2.7
- Networkx

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

# Building Decaf on Ubuntu 17.04 from scratch

The following instructuctions install Decaf and its dependencies on a new image of Ubuntu 17.04. The dependencies will be installed in ```$HOME/software/install``` and the Decaf project in ```$HOME/Decaf/install```. The paths and package might be modificed to fit your current environment or distribution.

```
#Install from a Ubuntu 17.04
#GCC version : 6.3.0


sudo apt-get install git
sudo apt-get install cmake #version 3.6
sudo apt-get install python-pip # version 2.7.13

# Minimum version: 1.8
sudo apt-get install openmpi-common
sudo apt-get install openmpi-bin
sudo apt-get install libopenmpi-dev

cd $HOME
mkdir software
cd software
mkdir install

export PATH=$HOME/software/install/include:$HOME/software/install/bin:$PATH
export LD_LIBRARY_PATH=$HOME/software/install/lib:$LD_LIBRARY_PATH

# Dependencies installation

#Boost
cd $HOME/software
wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz
tar -xf boost_1_64_0.tar.gz
cd boost_1_64_0
./bootstrap.sh --prefix=$HOME/software/install/
./b2 install

#NetworkX
pip install networkx

# Decaf build
cd $HOME
mkdir Decaf
cd Decaf
mkdir source build install
cd source
git clone https://bitbucket.org/tpeterka1/decaf.git .
cd ../build
cmake ../source/ -DCMAKE_INSTALL_PREFIX:PATH=$HOME/Decaf/install
make -j4 install

export LD_LIBRARY_PATH=$HOME/Decaf/install/lib:$LD_LIBRARY_PATH
export DECAF_PREFIX=$HOME/Decaf/install/
```

# Testing the installation on a local machine

Decaf provides several examples show casing simple workflows. Assuming Decaf was installed following the previous instructions, run the following commands:
```
# Testing examples
# All the examples should complete
cd $HOME/Decaf/install/examples/direct
python linear-2nodes.py
./linear2.sh

cd $HOME/Decaf/install/examples/direct
python linear-3nodes.py
./linear3.sh

cd $HOME/Decaf/install/examples/direct
python cycle-4nodes.py
./cycle.sh
```
# Testing the installation on a distribued machine

Certain python scripts from the examples take extra arguments to specify the number of cores and their respective hosts. The following gives an example to use the linear-2nodes examples in a distributed context:

```
# The linear-2nodes requires 8 procs (4 prod, 2 link, 2 cons)
cd $HOME/Decaf/install/examples/direct
echo -e "node1\nnode1\nnode1\nnode1\nnode2\nnode2\nnode3\nnode3" > hostfile.txt
python linear-2nodes-topo1.py --np 8 --hostfile hostfile.txt
./linear2.sh
```

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

