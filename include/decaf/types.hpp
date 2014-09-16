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
  DECAF_WORLD_COMM,
  DECAF_NUM_COMM_TYPES,
};

enum Error
{
  DECAF_OK,
  DECAF_COMM_SIZES_ERR,
  DECAF_NUM_ERRS,
};

#endif
