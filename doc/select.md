# Selection and Aggregation

Selection, distribution, and aggregation is performed as shown in the Figure below.

![Selectors](https://bitbucket.org/tpeterka1/decaf/raw/master/doc/figs/selectors.png)

From left to right, the data movement proceeds as follows.

- In the
producer, the user defines a custom selector function that does any
local filtering of the data.

- Decaf then automatically distributes the data between _i_ producer nodes and _j_ dataflow nodes (see data.md for details).

- Once in the dataflow, the user programs one or more rounds of aggregation using patterns similar to DIY's merge reduction, swap reduction, and neighbor communication.

- At the end of the dataflow, decaf again automatically redistributes the data between _j_ dataflow nodes and _k_ consumer nodes.

This scheme provides for a complete reverse steering dataflow if desired (symmetrical capability) with the addition of an optional selector on the consumer and reversed positions of the two distributor stages.
