# M:N Distribution

Currently (re)distribution from _M_ sources to _N_ destinations is done in contiguous fashion. See the figure below.

![Distribution](https://bitbucket.org/tpeterka1/decaf/raw/master/doc/figs/distribute-sources-dests.png)

Four functions in the Comm class compute, for the current rank, the number of outputs (destinations) to whom it should send, the starting rank (destination) of the first destination, the number of inputs (sources) from whom it should receive, and the starting rank of the first input (source) according to the figure above.
