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
        int prod_nsteps
        int con_interval

cdef extern from "../examples/direct/direct.cpp":
    void run(DecafSizes& decaf_sizes)

def pyrun(pyDecafSizes):
    cdef DecafSizes sizes
    sizes.prod_size    = pyDecafSizes.prod_size
    sizes.dflow_size   = pyDecafSizes.dflow_size
    sizes.con_size     = pyDecafSizes.con_size
    sizes.prod_start   = pyDecafSizes.prod_start
    sizes.dflow_start  = pyDecafSizes.dflow_start
    sizes.con_start    = pyDecafSizes.con_start
    sizes.prod_nsteps  = pyDecafSizes.prod_nsteps
    sizes.con_interval = pyDecafSizes.con_interval
    run(sizes)
