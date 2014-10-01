//---------------------------------------------------------------------------
//
// decaf typedefs, enums
//
// Tom Peterka
// Argonne National Laboratory
// 9700 S. Cass Ave.
// Argonne, IL 60439
// tpeterka@mcs.anl.gov
//
//--------------------------------------------------------------------------

#ifndef DECAF_TYPES_HPP
#define DECAF_TYPES_HPP

#include <stdio.h>

enum Decomposition
{
  DECAF_ROUND_ROBIN_DECOMP,
  DECAF_CONTIG_DECOMP,
  DECAF_ZCURVE_DECOMP,
  DECAF_NUM_DECOMPS,
};

enum CommType
{
  DECAF_PRODUCER_COMM,
  DECAF_CONSUMER_COMM,
  DECAF_DATAFLOW_COMM,
  DECAF_PROD_DFLOW_COMM,
  DECAF_CON_DFLOW_COMM,
  DECAF_WORLD_COMM,
  DECAF_NUM_COMM_TYPES,
};

enum Error
{
  DECAF_OK,
  DECAF_COMM_SIZES_ERR,
  DECAF_NUM_ERRS,
};

struct DecafSizes
{
  int prod_size; // size of producer communicator
  int con_size; // size of consumer communicator
  int dflow_size; // size of dataflow communicator
  int nsteps; // number of dataflow (consumer) time steps to execute (<= producer time steps)
};

void
all_err(int err_code)
{
  switch (err_code) {
  case DECAF_OK :
    break;
  case DECAF_COMM_SIZES_ERR :
    fprintf(stderr, "Decaf error: Group sizes of producer, consumer, and dataflow exceed total "
            "size of world communicator\n");
    break;
  default:
    break;
  }
}

#endif
