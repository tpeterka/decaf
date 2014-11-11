# Tom's Notes

--------------------------------

10/24/14

Todo:

- Check/fix number of items received, values of items received, sum inconsistent between runs and between tests of different overlap

For Hadrien:

- Nan will send SPH code in about a week or so, follow up
    - Is more expensive than I thought, takes around 3 minutes for 1e5 particles
- Updated to new HACC
    - pull new version from https://svn.alcf.anl.gov/repos/DarkUniverse/hacc/trunk/nbody/
    - understand new config file
    - TODO: get new API from HACC to to get alive particles; Hadrien is sorting them himself (should not be necessary)

For Lokman, Florin:

- copied DIY datatype code into decaf
- generated example datatypes for particles, delaunay, regular grid
- ask Florin about the need to use MPI_Get_address for typemaps
- discuss how to distribute m to n using the datatype
- discuss semantic higher-level information (is it necessary, or is layout all we want to specifiy; sematics for fault tolerance are needed, however (data criticality))
- Ask Florin about remaining allocated requests; Comm:flush() should have cleared them all
- will need a separate decaf communicator for intradataflow communication (DIY in the dataflow)
- will need decaf to be a separate executable (a main) when producer, dataflow, consumer are separate programs

Lokman:

- Will work on high-level API for defining typemaps
- Working on high-level API for launching decaf (configurations for executables, spawining processes

With Florin:

- think about threading in decaf
    - want to support damaris-like spare thread capability
    - user should choose between time and space tradeoff when overlapping communicators
    - as a starting point, we have an example of spare thread in DIY
    - the difference is that now the producer-dataflow and dataflow-consumer are sharing a communicator

Guillaume:

Model includes:

- Pipelining
- Buffering for flow control and resilience
- Replication for resilience (amount of replication = number of nodes, whether needs to adapt dynamically based on evolving patterns of errors) and tradeoff between buffering and replication
- SDC checks (whether to use an auxiliary method, whether and at what frequency to sample, vs check every result in space, time)
- Aggregation
    - black box for custom aggregator function
    - packet receipt may be wait for all to arrive or be pipelined with aggregator function
- Redistribution (may not be much to model there)

Objective:

- throughput is one objective
- latency is another
- others can be power, memory, etc.

--------------------------------

11/7/14

## SDC

### Three ways to check for errors:

- Spatial correlation
- Temporal correlation
- Auxiliary method

### Auxiliary method

- For errors occuring during the analysis (eg. tessellation - density estimation) that the auxiliary method performs.
- Not for errors occuring earlier, ie., inputs to the analysis
- Idea: learn the function f = (true_method - auxiliary_method)(input_data). Note, the - operator can be any distance metric (eg., ratio).
- When f_measured != f_expected, something is wrong. Note, the != comparison can represent any measure of distance.

### Test data

- HACC particles
- NFW, CNFW
- halo particles

### Questions for Slim

- Notes, record of work
    - scripts to compute errors
    - code to inject errors
- Modeling errors between CIC and TESS
    - scripts for machine learning to correlate errors
    - taken from the web but did not couple in the workflow yet
- Swift workflow scripts
    - did not complete
    - Start over with Justin

### Hadrien

- will change NGP to CIC in small tess cells
- will downsample CIC grid

