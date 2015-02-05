# Installing, building, running LAMMPS

## Dependencies

- FFTW 2.1.5 (not compatible with 3.3.4)
- jpeg and/or png depending whether -DLAMMPS_JPEG or -DLAMMPS_PNG is defined in the makefile below

## Download

- Download tarball from lammps.sandia.gov
- untar, assume for the following that the top level directory is called lammps

## Edit the makefile

- cd lammps/src/Make
- edit Makefile.mac_mpi (for building on a mac with mpi; other sample makefiles are included)
    - paths below are for my machine; yours will vary

```
CC =		mpicxx
LINK =		mpicxx
FFT_INC =       -DFFT_FFTW -I/Users/tpeterka/software/fftw-2.1.5/include
FFT_PATH =	-L/Users/tpeterka/software/fftw-2.1.5/lib
FFT_LIB =	-lfftw

# JPEG and/or PNG library, OPTIONAL
# only needed if -DLAMMPS_JPEG or -DLAMMPS_PNG listed with LMP_INC

JPG_INC =       -I/opt/local/include
JPG_PATH = 	-L/opt/local/lib
JPG_LIB =	-ljpeg
```
## Make

```
cd lammps/src
make mac_mpi
```
## Running an example

```
cd lammps/examples/melt
```

edit [in.melt](in.melt)

- lj is Lennard-Jones units
- fix 1 all nve signifies keep number, velocity, energy constant (vary temperature)
- dump 1 all custom 50 melt.dump id type x y z vx vy vz signifies a custom dump every 50 steps of position and velocity in the file melt.dump (one ascii file for entire time series)
- thermo 50 signifies dump temperature, pressure every 50 steps (to the screen)
- run 250 signifies run 250 steps
- 1 fs (femtosecond) is the default time step, configurable

```
cp lammps/src/lmp_mac_mpi lammps/examples/melt
cd lammps/exmaples/melt
./lmp_mac_mpi < in.melt
```
## Read into VMD or ParaView (VMD preferred)

