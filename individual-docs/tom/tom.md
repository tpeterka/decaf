# Tom's Notes

--------------------------------

10/24/14

Todo:

- Check/fix number of items received, values of items received, sum inconsistent between runs and between tests of different overlap

For Hadrien:

- Nan will send SPH code in about a week or so, follow up
    - Is more expensive than I thought, takes around 3 minutes for 1e5 particles
- Will get a movie of the HACC -> tess -> dense -> plots workflow for January PI meeting

For Lokman, Florin:

- Install HACC instructions into decaf wiki
- Lokman or Florin to implement data distribution
- discuss semantic higher-level information (is it necessary, or is layout all we want to specifiy; sematics for fault tolerance are needed, however (data criticality))
- will need a separate decaf communicator for intradataflow communication (DIY in the dataflow)
- will need decaf to be a separate executable (a main) when producer, dataflow, consumer are separate programs

Lokman:

- Implement data distribution using (offset,length) list
- Work on high-level API for defining typemaps
- Work on high-level API for launching decaf (configurations for executables, spawining processes
- Implement CM-1 + ITL + libsimVisit in Decaf

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

- Change NGP to CIC in small tess cells; did I get this patch and install it?
- Working on adaptive CIC (TSC); will get patch and install; compare accuracy in the dense paper
- Fiexed bug in tese-dense example, Tom to get it and patch into dense.

--------------------------------

11/25/14

## Data distribution

- Will need to think about custom redistribution functions in addition to the automatic datatype distribution. Use case: graph or unstructured mesh (tess) will require adding additional neighbors so that local consistency is preserved.

--------------------------------

3/12/15

## General todos / next steps

- Update my HACC example per Lokman's instructions
- Add dfg in networkx to my python script and generate decaf for two links of dfg
- Test lammps-workflow with overlapping communicators
- Add buffering (Florin)

## Matthieu next steps

- multinode DAG interface in python
    - provide sizes once per node and link
    - convert to decaf sizes for each dataflow
- Integrate with swift in addition to python
- Finish datatype splitting / merging
- Relax synchronization
- Multicore
- C++11? eg., initialization of decaf sizes in decaf constructor
- namespace conflict with openMPI?
- JLESC?
