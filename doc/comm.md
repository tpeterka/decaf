# Communicators

The communicator object is an abstraction for transport layer (for
example, MPI) communicators. A communicator can be created either by
its type or by a continuous range of ranks. The size and rank within a
communicator can be queried anytime.

The world communicator provided by the user is divided into three
different types of processes: producer, dataflow, and consumer (and
optionally other if more processes are left over). The user is
expected to manage his or her own producer and consumer communicators
as needed to execute produce and consumer intracommunication.

Decaf creates 2 communicators
(prod_dflow_comm_ and dflow_con_comm_) as shown in Figure

![Decaf communicators](https://bitbucket.org/tpeterka1/decaf/raw/master/doc/figs/comms.png)

that overlap the producer with
dataflow, and dataflow with consumer, respectively.

In terms of process rank order in the world communicator, the producer
ranks, dataflow ranks, and consumer ranks are monotonically
nondecreasing. By specifying the starting world rank of the producer,
dataflow, and consumer, these ranks may be disjoint as in the upper
part of the figure, partially overlapping as in the middle part of the figure, or
entirely in the same set of ranks, as shown in the lower part of the
figure.

The figure below shows three examples of completely disjoint, partially overlapping, and completely overlapping producer, dataflow, and consumer ranks. Many other combinations are possible with the ability to control at which world rank the dataflow and consumer begin. Also, note that in some cases a the source and destination are the same. In these cases the data are simply copied and not actually sent in a message. Also note that the same rank may need part of its data from a remote source and part from the same source.

![Examples of overlap](https://bitbucket.org/tpeterka1/decaf/raw/master/doc/figs/overlap.png)

See [decaf/comm.hpp](../include/decaf/comm.hpp).
