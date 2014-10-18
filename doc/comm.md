# Communicators

The communicator object is an abstraction for transport layer (for
example, MPI) communicators. A communicator can be created either by
its type or by a continuous range of ranks. The size and rank within a
communicator can be queried anytime. Starting with the world
communicator provided by the user, decaf creates 2 communicators
(prod_dflow_comm_ and dflow_con_comm_) as shown in Figure

![Decaf communicators](../doc/figs/comms.png?raw=true)
![Decaf communicators](https://bitbucket.org/tpeterka1/decaf/src/1fdb0c7285168e4391f1b27cff0f65e4286e56fc/doc/figs/comms.png?raw=true)
that overlap the producer with
dataflow, and dataflow with consumer, respectively.

See [decaf/comm.hpp](../include/decaf/comm.hpp).
