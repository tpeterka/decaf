# Data Model

The low-level data model and its distribution from _i_ producer nodes to _j_ dataflow nodes is shown in the figure below.

![Selectors](https://bitbucket.org/tpeterka1/decaf/raw/master/doc/figs/data.png)

The data model works as follows:

- The producer defines some high-level language object such as a struct or class in terms of a typemap of elemental objects.

- Each element is a contiguous region of data layout consisting of an offset and count of some base datatype.

- Decaf automatically distributes the elements to a different number of nodes. Element quantities can be subdivided, but not element types.

- The distribution type is contiguous.

- Distribution can occur for both the forward (analysis) and reverse (steering) directions.

- These datatypes only define layout, not semantics. A higher level wrapper can define more logical data descriptions.

- Pipeline chunking will use the same mechanism.
