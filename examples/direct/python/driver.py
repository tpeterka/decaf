# set your options here

class DecafSizes:
    prod_size    = 4      # size of producer communicator
    dflow_size   = 2      # size of dataflow communicator
    con_size     = 2      # size of consumer communicator
    prod_start   = 0      # starting rank of producer communicator in the world
    dflow_start  = 4      # starting rank of dataflow communicator in the world
    con_start    = 6      # starting rank of consumer communicator in the world
    prod_nsteps  = 2      # total number of time steps
    con_interval = 1      # run consumer every so often

# do not edit below this point

import driver
driver.pyrun(DecafSizes)
