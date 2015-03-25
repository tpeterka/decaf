# Python interface

To use the (preferred) python interface, eg. to wrap an example such as direct.cpp, the following steps are performed using cython.

- A short .pyx (python extensions) cython source file is written (eg. [examples/direct/python/driver.pyx](../examples/direct/python/driver.pyx)) This file maps relevant c++ objects in direct.cpp to python.

- The file driver.pyx is compiled by cython to produce py_driver.cxx. This file is the python wrapper to the original decaf c++ objects in direct.cpp, but in c++ language.

- The file py_driver.cxx is compiled and linked into a shared object libpy_driver.so. Shared objects can be imported into python as python modules.

- A short python script driver.py is written by the user (or modified from existing). This script sets workflow execution parameters, imports the libpy_driver.so module, and runs it. An example script is [examples/direct/python/driver.py](../examples/direct/python/driver.py).
