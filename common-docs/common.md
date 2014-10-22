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
        - I have a 128^3 dataset (single time step)
        - There are a few 1024^3 datasets (single time steps) on Vesta or Mira

### In situ running HACC and calling tess, dense

- I have an old version of HACC on the mcs network at ~tpeterka/rru2
- See rru2/simulation/driver_pm.cxx
- Tessellate() calls tess (probably needs updating for current tess API)
- Instructions for building, configuring, and running HACC are somewhere

## Installing and running SWIFT

- Justin has a Swift/tess executable that Slim was starting to use
- The new swift preview release 0.6.2 is available [here](http://swift-lang.org/Swift-T/downloads/exm-0.6.2.tar.gz)
- Swift notes for integrating with tess are [here](https://docs.google.com/document/d/119ThFfmJXIy4H4LwD4bTu0rPdiADBRS525X09zCkGCg)

