#! /bin/bash

mpiexec -np 2 ./prod_c_gromacs : -np 1 ./dflow_gromacs : -np 1 ./treatment
