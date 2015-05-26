FIND_PATH(FFS_INCLUDE_DIR ffs.h
  ${FFS_PREFIX}/include
  /usr/include/
  /usr/local/include
  /opt/local/include
  /sw/include
)
FIND_LIBRARY(FFS_LIBRARY ffs
  ${FFS_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

IF(FFS_INCLUDE_DIR AND FFS_LIBRARY) 
  SET(FFS_FOUND 1 CACHE BOOL "Found ffs library")
ELSE(FFS_INCLUDE_DIR AND FFS_LIBRARY) 
  SET(FFS_FOUND 0 CACHE BOOL "Not found ffs library")
  IF (FFS_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "FFS library not found. Please install and provide the FFS_PREFIX variable to help finding the library")
  ENDIF(FFS_FIND_REQUIRED)
ENDIF(FFS_INCLUDE_DIR AND FFS_LIBRARY) 

MARK_AS_ADVANCED(
  FFS_INCLUDE_DIR
  FFS_LIBRARY
  FFS_FOUND
)


