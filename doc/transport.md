# Transport Layer

Decaf is intended to be used with various lower level transport layers, such as MPI or Nessie. Therefore, no low-level transport-specific code is exposed in the top-level decaf interface. The choice of transport layer is controlled by the configure-time CMake variable. Currently, the only available transport layer is MPI, and transport_mpi=on should be set in the CMake command line.

The transport-specific code is in /decaf/transport. A directory exists for each transport layer, currently only mpi. /decaf/transport/mpi contains the implementation of the communicator class and typdefs for various MPI objects that have been abstracted in the higher level decaf classes.
