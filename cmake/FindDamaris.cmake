FIND_PATH(DAMARIS_INCLUDE_DIR Damaris.h HINTS
  ${DAMARIS_PREFIX}/include/
  /usr/include/decaf
  /usr/local/include/decaf
  /opt/local/include/decaf
  /sw/include/decaf
)
FIND_LIBRARY(DAMARIS_LIBRARY NAMES damaris HINTS
  ${DAMARIS_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

IF(DAMARIS_INCLUDE_DIR AND DAMARIS_LIBRARY)
  SET(DAMARIS_FOUND 1 CACHE BOOL "Found Damaris library")
ELSE(DAMARIS_INCLUDE_DIR AND DAMARIS_LIBRARY)
  SET(DAMARIS_FOUND 0 CACHE BOOL "Not found Damaris library")
  IF (DAMARIS_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Damaris library not found. Please install and provide the DAMARIS_PREFIX variable to help finding the library")
  ENDIF(DAMARIS_FIND_REQUIRED)
ENDIF(DAMARIS_INCLUDE_DIR AND DAMARIS_LIBRARY)

MARK_AS_ADVANCED(
  DAMARIS_INCLUDE_DIR
  DAMARIS_LIBRARY
  DAMARIS_FOUND
)
