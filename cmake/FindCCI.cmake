INCLUDE(FindPackageHandleStandardArgs)

FIND_PATH(CCI_INCLUDE_DIR cci.h HINTS
  ${CCI_PREFIX}/include/
  /usr/include/
  /usr/local/include/
  /opt/local/include/
  /sw/include/
)

FIND_LIBRARY(CCI_LIBRARY NAMES cci HINTS
  ${CCI_PREFIX}/lib
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  /sw/lib
)

find_package_handle_standard_args(CCI DEFAULT_MSG
    CCI_INCLUDE_DIR
    CCI_LIBRARY
)

if(CCI_INCLUDE_DIR)
    SET(CCI_FOUND 1 CACHE BOOL "Found CCI libraries")
endif(CCI_INCLUDE_DIR)

MARK_AS_ADVANCED(
  CCI_INCLUDE_DIR
  CCI_LIBRARY
  CCI_FOUND
)


