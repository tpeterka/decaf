# Common Documents

# Tess - dense - swift integration and workflow (10/22/14)

## Installing and running tess and dense

- See the README [here](https://bitbucket.org/diatomic/tess)

### Generating synthetic test particles

- See the README [here](https://bitbucket.org/diatomic/tess)

### Postprocessing reading particles

- Serial read and broadcast
        - See tess/examples/post-tess
        - Supports ascii, binary float, binary double
- Parallel read
        - See tess/examples/pread-voronoi
        - Supports HDF5
- Paralel read HACC Generic I/O particles
        - See tess/examples/hacc-post-tess
        - I have a 128^3 dataset (single time step) on the mcs network in ~tpeterka/software/tess/1-dev/examples/hacc-post-tess/m000.mpicosmo.499
        - There are a few 1024^3 datasets (single time steps) on Cetus/Mira in /projects/SSSPPg/tpeterka/STEP68/m003.full.mpicosmo.68, /projects/SSSPPg/tpeterka/STEP247/m003.full.mpicosmo.247, /projects/SSSPPg/tpeterka/STEP499/m003.full.mpicosmo.499 for timesteps 68, 247, and 499 respectively.

### In situ running HACC and calling tess, dense

- I have an old version of HACC on the mcs network at ~tpeterka/rru2 (run from my soprano machine or from any of the mcs compute servers, not the login node)
- See rru2/simulation/driver_pm.cxx
- Tessellate() calls tess (probably needs updating for current tess API)

### HACC dependencies

- Install fftw-3.3: (rru2 requires v.3.3)
- configure fftw with --enable-mpi

### Build

- see current version of env/bashrc.linux-tess and env/bashrc.bgp-tess for my build options
- for linux: source env/bashrc.linux-tess
- cd cpu
- make
- If build fails, try make clean; make
- If that fails, try performing the make clean from the top level:
- cd ~/rru2
- make clean
- cd cpu
- make

### Run

- cd ~/rru2/testing/tess
- The bash script MPI_TEST has run commands for a variety of machines. Feel free to copy and modify it to suit your purposes. Run it as: ./MPI_TEST
- You can copy MPI_TEST, indat.txt and cmbM000.tf

## Installing and running SWIFT

- Justin has a Swift/tess executable that Slim was starting to use
- The new swift preview release 0.6.2 is available [here](http://swift-lang.org/Swift-T/downloads/exm-0.6.2.tar.gz)
- Swift notes for integrating with tess are [here](https://docs.google.com/document/d/119ThFfmJXIy4H4LwD4bTu0rPdiADBRS525X09zCkGCg)
- One patch needs to be applied to tess, where the first argument to tess is the MPI communicator. (I ought to fix this permanentely in tess.)
