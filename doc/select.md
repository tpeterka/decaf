# Selection and Aggregation

Selection and aggregation is performed with two user-defined selector
functions, prod_selector() and dflow_selector(). The former runs in
the producer, and the latter runs in the dataflow. Each selector
function (in each process of the respective communicators) has the
opportunity to filter each incoming datatype object before passing it
on to each process of the the subsequent communicator. In this 2-step
selection process, both selection and aggregation can be
accomplished. See Figure
![Selectors](https://bitbucket.org/tpeterka1/decaf/src/doc/figs/selectors.png).

See Decaf::put() for each producer node calling prod_selector() prior to sending to each dataflow node. See Decaf::dataflow() for each dataflow node receiving data from each producer node and calling dflow_selector() prior to sending to each consumer node.
