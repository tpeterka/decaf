INCLUDE_DIRECTORIES(.)

include_directories(${Boost_INCLUDE_DIRS})

set(bredala_transports "")
if(${transport_mpi})
    list(APPEND bredala_transports bredala_transport_mpi)
endif(${transport_mpi})
if(${transport_cci})
    list(APPEND bredala_transports bredala_transport_cci)
endif(${transport_cci})

ADD_LIBRARY(dca SHARED cdecaf.cpp)
TARGET_LINK_LIBRARIES(dca ${Boost_LIBRARIES} ${bredala_transports} bredala_datamodel manala bca decaf)
SET_TARGET_PROPERTIES(dca PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# DEPRECATED, cmake doesn't like the LIBRARY keyword
# INSTALL(TARGETS dca LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
INSTALL(TARGETS dca DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

