# Use Case: HACC-TESS-DENSE Data Pipline

## HACC Compilation

HACC is not under an open source licence, please contact Tom to get it. In case you have access to ANL intranet, Tom is keeping an up-to-date version of HACC on his home directory at /homes/tpeterka/hacc/. Otherwise, I made  a packaged version excluding  svn-related directories that you can get from /homes/lrahmani/hacc-new.tar.gz. 
Get the hacc-new.tar.gz package from ANL internal network, extract it:


```
#!bash

scp <username>@login.mcs.anl.gov:/homes/lrahmani/hacc-new.tar.gz .
tar -xzvf hacc-new.tar.gz
rm hacc-new.tar.gz
cd hacc/
export HACC_RT=$(pwd)

```

Hacc is structured into different modules in addition to the nbody solver one. We need to build those modules separately:

- genericio

```
#!bash

cd $HACC_RT/genericio
mkdir build && cd build
cmake .. && make
# important build files: libGenericIO.a

```
- cosmotools

First we need to change the CMakeList.txt file at: cosmotools/algorithms/halofinder to add "FOFDistribute.cxx" to the variable "HALOFINDER_SRC".

```
#!cmake

## Set halofinder sources
set(HALOFINDER_SRC
    BHTree.cxx
    bigchunk.cxx
    ChainingMesh.cxx
    CosmoHaloFinder.cxx
    CosmoHaloFinderP.cxx
    dims-local-init.cxx
    dims.cxx
    FOFHaloProperties.cxx
    GridExchange.cxx
    HaloCenterFinder.cxx
    HaloFinderInput.cxx
    InitialExchange.cxx
    Message.cxx
    ParticleDistribute.cxx
    ParticleExchange.cxx
    Partition.cxx
    SimpleTimings.cxx
    SODHalo.cxx
    SUBFIND.cxx
    SubHaloFinder.cxx
    Timer.cxx
    Timings.cxx
#    SubHaloFinderDev.cxx
    FOFDistribute.cxx # add this line
    )

```



Then, we compile the code

```
#!bash
cd $HACC_RT/cosmotools/
mkdir build && cd build
cmake .. -DGENERIC_IO_INCLUDE_DIR=../../genericio/ -DGENERIC_IO_LIBRARIES=../../genericio/build/libGenericIO.a
make
# This will generate a library, ,that will be used by nbody simulation
# Important build files: libs/libcosmotools.a

```
- build hacc
    - hacc depend also on an external library, fftw3 (Fastest Fourier Transformation in the West)

```
#!bash

cd $HACC_RT
wget http://www.fftw.org/fftw-3.3.4.tar.gz -P /tmp/
tar -xzvf /tmp/fftw-3.4.4.tar.gz && cd fftw-3.3.4
mkdir build && cd build
../configure  --enable-openmp
make
```

hacc  don’t use autotools or cmake, instead it release on shell envirnement variables. You can find examples in the env/ directory. Here is the env file we will be using (bashrc.linux-tess):


```
#!bash
# file bashrc.linux-tess
export HACC_PLATFORM="linux"
export HACC_OBJDIR="${HACC_PLATFORM}"

#export HACC_MPI_CFLAGS="-O3 -Wall -std=c99"
export HACC_MPI_CFLAGS="-g -Wall -std=c99"
export HACC_MPI_CC="mpicc"

#export HACC_MPI_CXXFLAGS="-O3 -Wall -Wno-deprecated"
export HACC_ROOT="${HOME}/repos/tess/hacc"
export COSMOTOOLS_LIBRARY_DIR="${HACC_ROOT}/cosmotools/build/libs/"
export GENERICIO_LIBRARY_DIR="${HACC_ROOT}/genericio/build/"
export TESS_LIBRARY_DIR="${HOME}/repos/tess/build/src/"
export DIY_LIBRARY_DIR="${HOME}/local/diy/lib"
export DIY_INCLUDE_DIR="${HOME}/locaal/diy/include/"
export QHULL_INCLUDE_DIR="${HOME}/local/include/libqhull"
export DEFAULT_ROOT="${HOME}/local/"
export DEFAULT_LIB_DIR="${DEFAULT_ROOT}/lib/"
export DEFAULT_INC_DIR="${DEFAULT_ROOT}/include/"

#export HACC_MPI_CXXFLAGS="-g -Wall -Wno-deprecated -fopenmp -I${HACC_ROOT}/repos/tess/hacc/voronoi/src/lib -I${HOME}/local/diy/include -I${HOME}/local/include/libqhull -I${HOME}/local/include/ -DDIY -I${HACC_ROOT}/cosmotools/algorithms/halofinder/ -I${HACC_ROOT}/cosmotools/common/"
export HACC_MPI_CXXFLAGS="-g -Wall -Wno-deprecated -fopenmp  -I${DEFAULT_INC_DIR}  -I${DIY_INCLUDE_DIR} -I${QHULL_INCLUDE_DIR} -DDIY -I${HACC_ROOT}/cosmotools/algorithms/halofinder/ -I${HACC_ROOT}/cosmotools/common/"
export HACC_MPI_CXX="mpicxx"

export HACC_MPI_LDFLAGS="-lm -L${TESS_LIBRARY_DIR} -ltess -L${DEFAULT_LIB_DIR} -lqhullstatic -L${DIY_LIBRARY_DIR} -ldiy -lz -lpnetcdf -L${GENERICIO_LIBRARY_DIR} -lGenericIO -L${COSMOTOOLS_LIBRARY_DIR}"

export FFTW_MAJOR_VERSION=3

# needed by nbody/initializer/Makefile
export COSMOTOOLS_MODULE=${HACC_ROOT}/cosmotools

export FFTW_HOME=${DEFAULT_ROOT}
export FFTW_INCLUDE=${DEFAULT_ROOT}/include
export CPATH=${DEFAULT_ROOT}/include:${CPATH}
if [ -f ${DEFAULT_ROOT}/lib64 ]
then
export LD_LIBRARY_PATH=${DEFAULT_ROOT}/lib64:${LD_LIBRARY_PATH}
else
export LD_LIBRARY_PATH=${DEFAULT_ROOT}/lib:${LD_LIBRARY_PATH}
fi
export INFOPATH=${DEFAUKT_ROOT}/share/info:${INFOPATH}
export MANPATH=${DEFAULT_ROOT}/share/man:$MANPATH
export PATH=${DEFAULT_ROOT}/bin:${PATH}

```
FIXME: add a link to download the file

Once we have setup all bash variables correctly, we can now build the hacc's nbody solver


```
#!bash
cd $HACC_RT/nbody/cpu 
source ../env/bashrc.linux-tess
make


```

## Compiling & Runing TESS & DENSE 

[See Tom's instructions](https://bitbucket.org/tpeterka1/decaf/wiki/common-docs/common)

FIXME: add some details

## Running HACC

HACC needs two input files for hacc to run: "ic.hcosmo.x86.0" and "../smalvol/indat.txt". Use the script provided here (based on Tom's MPI_TEST) to run correctly HACC:

```
#!bash

#!/bin/bash
# file MPI_TEST

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
# All rights reserved. May not be used, modified, or copied
# without permission
#
#----------------------------------------------------------------------------
#ARCH=MAC_OSX
ARCH=LINUX
#ARCH=BGP
#ARCH=FUSION
#ARCH=XT
#ARCH=XE

# number of procs
num_procs=16

# number of BG/P nodes for vn mode
num_nodes=$[$num_procs / 4]
if [ $num_nodes -lt 1 ]; then
    num_nodes=1
fi

# executable
if [ "$ARCH" = "LINUX" ]; then
exe=${HOME}/repos/tess/hacc/nbody/cpu/linux/hacc_pm
fi
if [ "$ARCH" = "BGP" ]; then
exe=${HOME}/repos/tess/hacc/nbody/cpu/bgp/hacc_pm
fi

# program arguments
indat="../smallvol/indat"
tf="ic.hcosmo.x86.0"
out="output/particules"
opts="RECORD ROUND_ROBIN"

args="$indat $tf $out $opts"

#------
#
# run commands
#

if [ "$ARCH" = "MAC_OSX" ]; then

mpiexec -n $num_procs $exe $args

fi

if [ "$ARCH" = "LINUX" ]; then

echo "mpiexec  -n $num_procs $exe $args"
mpiexec  -n $num_procs $exe $args

fi

if [ "$ARCH" = "BGP" ]; then

cqsub -n $num_procs -c $num_procs -p UltraVis -t 30 -m smp $exe $args

fi

if [ "$ARCH" = "FUSION" ]; then

mpiexec $exe $args

fi

if [ "$ARCH" = "XT" ]; then

cd /tmp/work/$USER
aprun -n $num_procs $exe $args

fi

if [ "$ARCH" = "XE" ]; then

aprun -n $num_procs $exe $args

fi

```

Running HACC:
```
#!bash

cd $HACC_RT/nbody/testing
mkdir tess/ && cd tess 
mkdir -p output && ./MPI_TEST
```
This will generates one file per iteration, each contains particle data generated by that iteration.
## Runing HACC, TESS and DENSE In Situ

TODO


