# Tom's Notes

10/24/14

Todo:

- Check/fix number of items received, values of items received, sum inconsistent between runs and between tests of different overlap
- Ask about remaing allocated requests; Comm:flush() should have cleared them all

For Hadrien:

- Nan will send SPH code in about a week or so, follow up
    - Is more expensive than I thought, takes around 3 minutes for 1e5 particles
- Update to new HACC
    - pull new version from https://svn.alcf.anl.gov/repos/DarkUniverse/hacc/trunk/nbody/
    - understand new config file

For Lokman, Florin:

- copy DIY datatype code into decaf
- generate example datatypes for particles, delaunay, regular grid
- discuss how to distribute m to n using the datatype
- discuss semantic higher-level information (is it necessary, or is layout all we want to specifiy; sematics for fault tolerance are needed, however (data criticality))

With Florin:

- think about threading in decaf
    - want to support damaris-like spare thread capability
    - user should choose between time and space tradeoff when overlapping communicators
    - as a starting point, we have an example of spare thread in DIY
    - the difference is that now the producer-dataflow and dataflow-consumer are sharing a communicator
