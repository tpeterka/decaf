#!/bin/bash

# directives for TITAN
#PBS -A csc242
#PBS -N test
#PBS -j oe
#PBS -l walltime=0:10:00,nodes=1

# directives for CORI and EDISON
#SBATCH -p debug
#SBATCH -N 1
#SBATCH -t 00:10:00
#SBATCH -J my_job
#SBATCH -o my_job.o%j

#----------------------------------------------------------------------------
#
# mpi run script
#
# Tom Peterka
# Argonne National Laboratory
# 9700 S. Cass Ave.
# Argonne, IL 60439
# tpeterka@mcs.anl.gov
#
#----------------------------------------------------------------------------
#ARCH=MAC_OSX
ARCH=LINUX
#ARCH=BGQ
#ARCH=FUSION
#ARCH=TITAN
#ARCH=EDISON
#ARCH=CORI

# number of procs
num_procs=8

# procs per node
ppn=1 # adjustable for BG/Q, allowed 1, 2, 4, 8, 16, 32, 64

# number of nodes
num_nodes=$[$num_procs / $ppn]
if [ $num_nodes -lt 1 ]; then
    num_nodes=1
fi

# executable
exe=./linear_2nodes

#------
#
# run commands
#

if [ "$ARCH" = "MAC_OSX" ]; then

mpiexec -l -n $num_procs $exe

#dsymutil $exe ; mpiexec -l -n $num_procs xterm -e gdb -x gdb.run --args $exe

#dsymutil $exe ; mpiexec -n $num_procs valgrind -q $exe

#dsymutil $exe ; mpiexec -n $num_procs valgrind -q --leak-check=yes $exe

fi

if [ "$ARCH" = "LINUX" ]; then

mpiexec -n $num_procs $exe

#mpiexec -n $num_procs xterm -e gdb -x gdb.run --args $exe

#mpiexec -n $num_procs valgrind -q $exe

#mpiexec -n $num_procs valgrind -q --leak-check=yes $exe

fi

if [ "$ARCH" = "BGQ" ]; then

qsub -n $num_nodes --env LD_LIBRARY_PATH=$LD_LIBRARY_PATH:DECAF_PREFIX=$DECAF_PREFIX --mode c$ppn -A SDAV -t 60 $exe

# how to trace ld paths for debugging dynamic libraries not being found
# qsub -n $num_nodes --env LD_LIBRARY_PATH=$LD_LIBRARY_PATH:LD_DEBUG=libs:LD_VERBOSE=1:LD_TRACE_LOADED_OBJECTS=1 --mode c$ppn -A SDAV -t 60 $exe

fi

if [ "$ARCH" = "EDISON" ]; then

srun -n $num_procs $exe

fi

if [ "$ARCH" = "CORI" ]; then

srun -C haswell -n $num_procs $exe

fi

if [ "$ARCH" = "TITAN" ]; then

cd /ccs/proj/csc242/decaf/install/examples/direct
date
aprun -n $num_procs $exe

fi
