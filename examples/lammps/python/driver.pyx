cdef extern from "decaf/decaf.hpp":
    pass

cdef extern from "decaf/types.hpp":
    struct DecafSizes:
        int prod_size
        int dflow_size
        int con_size
        int prod_start
        int dflow_start
        int con_start
        int con_nsteps

cdef extern from "../examples/lammps/lammps.cpp":
    void run(DecafSizes& decaf_sizes, int prod_nsteps, char* infile)

def pyrun(pyDecafSizes, prod_nsteps, infile):
    cdef DecafSizes sizes
    sizes.prod_size    = pyDecafSizes.prod_size
    sizes.dflow_size   = pyDecafSizes.dflow_size
    sizes.con_size     = pyDecafSizes.con_size
    sizes.prod_start   = pyDecafSizes.prod_start
    sizes.dflow_start  = pyDecafSizes.dflow_start
    sizes.con_start    = pyDecafSizes.con_start
    sizes.con_nsteps   = pyDecafSizes.con_nsteps
    run(sizes, prod_nsteps, infile)
