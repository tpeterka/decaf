#! /bin/bash

cd "/home/matthieu/Argonne/Decaf/build"
cmake ../source/decaf/ -DCMAKE_INSTALL_PREFIX:PATH=/home/matthieu/Argonne/Decaf/install -DLAMMPS_INCLUDE_DIR:PATH=/home/matthieu/Dependencies/lammps-10Feb15/src/ -DLAMMPS_LIBRARY:PATH=/home/matthieu/Dependencies/lammps-10Feb15/src/liblammps.so -DFFTW_LIBRARY:PATH=/home/matthieu/Dependencies/lib/libfftw.so -DFFTW_INCLUDE_DIR:PATH=/home/matthieu/Dependencies/include/
cd "/home/matthieu/Argonne/Decaf/source/decaf"
