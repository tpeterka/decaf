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

// data element for creating typemaps for datatypes
enum DispType
{
  DECAF_OFST,
  DECAF_ADDR,
  DECAF_NUM_DISP_TYPES,
};
struct DataMap {
  CommDatatype base_type; // existing datatype used to create this one
  DispType disp_type;     // diplacement is relative OFST or absolute ADDR
  int count;              // count of each element
  Address disp;           // starting displacement of each element in bytes
                          // OFSTs are from the start of the type and ADDRs are from 0x0
};

enum Decomposition
{
  DECAF_ROUND_ROBIN_DECOMP,
  DECAF_CONTIG_DECOMP,
  DECAF_ZCURVE_DECOMP,
  DECAF_NUM_DECOMPS,
};

// task types
typedef unsigned char TaskType;
#define DECAF_PROD      0x00
#define DECAF_DFLOW     0x01
#define DECAF_CON       0x02

// communicator types
typedef unsigned char CommType;
#define DECAF_OTHER_COMM      0x00
#define DECAF_PRODUCER_COMM   0x01
#define DECAF_DATAFLOW_COMM   0x02
#define DECAF_CONSUMER_COMM   0x04
#define DECAF_PROD_DFLOW_COMM 0x08
#define DECAF_DFLOW_CON_COMM  0x10

enum DecafError
{
  DECAF_OK,
  DECAF_COMM_SIZES_ERR,
  DECAF_NUM_ERRS,
};

struct DecafSizes
{
  int prod_size;    // size of producer communicator
  int dflow_size;   // size of dataflow communicator
  int con_size;     // size of consumer communicator
  int prod_start;   // size of producer communicator
  int dflow_start;  // size of dataflow communicator
  int con_start;    // size of consumer communicator
  int con_nsteps;   // number of consumer timesteps
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

#define DECAF_DEBUG_MAX 256

void 
all_dbg( FILE* channel, char* msg)
{
#ifdef DECAF_DEBUG_ON 
  fprintf(channel, "[DECAF-DEBUG] %s", msg);
#endif
}

#endif
