# Class Organization

So far, the top-level classes are:

## Decaf

The Decaf class partitions the process ranks into producer, dataflow, and consumer. It creates overlapping communicators for producer-dataflow and dataflow-consumer. These are explained in [Communicators](comm.md). It launches the dataflow for the appropriate ranks. The Decaf class also provides put and get functions to send data via the dataflow from producer to consumer.

## Data

The Data class stores the elementary datatype from which items are composed, and a vector of items of that datatype. Various access functions to query the number of items or get a pointer to them are provided.

## Comm

The Comm class is a wrapper for a set of ranks that communicate, as in an MPI communicator. Functions to create and query the size and rank are provided. Data can be sent and received to/from the communicator via put and get functions.
