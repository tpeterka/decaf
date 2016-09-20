FIND_PATH(DECAF_CXX_INCLUDE_DIR decaf.hpp HINTS
  ${DECAF_PREFIX}/include/decaf
  /usr/include/decaf
  /usr/local/include/decaf
  /opt/local/include/decaf
  /sw/include/decaf
)
FIND_LIBRARY(DECAF_CXX_DATA_MODEL_LIBRARY NAMES bredala_datamodel HINTS
  ${DECAF_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

FIND_LIBRARY(DECAF_CXX_DATA_TRANSPORT_LIBRARY NAMES bredala_transport HINTS
  ${DECAF_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

FIND_PATH(DECAF_C_INCLUDE_DIR bredala.h HINTS
  ${DECAF_PREFIX}/include/decaf/C
  /usr/include/decaf/C
  /usr/local/include/decaf/C
  /opt/local/include/decaf/C
  /sw/include/decaf/C
)
FIND_LIBRARY(DECAF_C_DATA_MODEL_LIBRARY NAMES bca HINTS
  ${DECAF_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

FIND_LIBRARY(DECAF_C_RUNTIME_LIBRARY NAMES dca HINTS
  ${DECAF_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

IF(DECAF_CXX_INCLUDE_DIR AND DECAF_CXX_DATA_MODEL_LIBRARY AND DECAF_CXX_DATA_TRANSPORT_LIBRARY AND DECAF_C_INCLUDE_DIR AND DECAF_C_DATA_MODEL_LIBRARY AND DECAF_C_RUNTIME_LIBRARY)
  SET(DECAF_FOUND 1 CACHE BOOL "Found decaf libraries")
  STRING(REGEX REPLACE "/decaf" "" DECAF_CXX_INCLUDE_DIR ${DECAF_CXX_INCLUDE_DIR})
  STRING(REGEX REPLACE "/decaf/C" "" DECAF_C_INCLUDE_DIR ${DECAF_C_INCLUDE_DIR})
ELSE(DECAF_CXX_INCLUDE_DIR AND DECAF_DATA_CXX_MODEL_LIBRARY AND DECAF_CXX_DATA_TRANSPORT_LIBRARY AND DECAF_C_INCLUDE_DIR AND DECAF_C_DATA_MODEL_LIBRARY AND DECAF_C_RUNTIME_LIBRARY)
  SET(DECAF_FOUND 0 CACHE BOOL "Not found decaf libraries")
  IF (DECAF_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Decaf library not found. Please install and provide the DECAF_PREFIX variable to help finding the library")
  ENDIF(DECAF_FIND_REQUIRED)
ENDIF(DECAF_CXX_INCLUDE_DIR AND DECAF_CXX_DATA_MODEL_LIBRARY AND DECAF_CXX_DATA_TRANSPORT_LIBRARY AND DECAF_C_INCLUDE_DIR AND DECAF_C_DATA_MODEL_LIBRARY AND DECAF_C_RUNTIME_LIBRARY)

MARK_AS_ADVANCED(
  DECAF_CXX_INCLUDE_DIR
  DECAF_CXX_DATA_MODEL_LIBRARY
  DECAF_CXX_DATA_TRANSPORT_LIBRARY
  DECAF_C_INCLUDE_DIR 
  DECAF_C_DATA_MODEL_LIBRARY 
  DECAF_C_RUNTIME_LIBRARY
  DECAF_FOUND
)


