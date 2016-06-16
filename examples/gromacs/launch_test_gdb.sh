#! /bin/bash

mpiexec -np 2 xterm -hold -e gdb -x gdb.run --args ./prod_c_gromacs : -np 1 xterm -hold -e gdb -x gdb.run --args ./dflow_gromacs : -np 1 xterm -hold -e gdb -x gdb.run --args ./treatment
