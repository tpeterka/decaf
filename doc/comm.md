# Communicators

The communicator object is an abstraction for transport layer (for
example, MPI) communicators. A communicator can be created either by
its type or by a continuous range of ranks. The size and rank within a
communicator can be queried anytime. Starting with the world
communicator provided by the user, decaf creates 2 communicators
(prod_dflow_comm_ and dflow_con_comm_) as shown in Figure
![Decaf communicators](https://bitbucket.org/tpeterka1/decaf/src/ba83607d929e4d6372835414f58fd9df0f591385/doc/figs/comms.png?at=master)
that overlap the producer with
dataflow, and dataflow with consumer, respectively.

See [decaf/comm.hpp](../include/decaf/comm.hpp).
