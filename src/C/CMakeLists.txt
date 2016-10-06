INCLUDE_DIRECTORIES(.)

find_package            (Boost COMPONENTS serialization REQUIRED)

if(Boost_FOUND)

  include_directories(${Boost_INCLUDE_DIRS})
  # DEPRECATED, don't force shared; let the user decide
  # ADD_LIBRARY(bca SHARED bredala.cpp)
  ADD_LIBRARY(bca bredala.cpp)
  TARGET_LINK_LIBRARIES(bca ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  SET_TARGET_PROPERTIES(bca PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  # DEPRECATED, cmake doesn't like the LIBRARY keyword
  # INSTALL(TARGETS bca LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)  
  INSTALL(TARGETS bca DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)  

  if(NOT ${bredala_only})
    # DEPRECATED, don't force shared; let the user decide
    # ADD_LIBRARY(dca SHARED cdecaf.cpp)
    ADD_LIBRARY(dca cdecaf.cpp)
    TARGET_LINK_LIBRARIES(dca ${Boost_LIBRARIES} bredala_transport bredala_datamodel bca)
    SET_TARGET_PROPERTIES(dca PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    # DEPRECATED, cmake doesn't like the LIBRARY keyword
    # INSTALL(TARGETS dca LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
    INSTALL(TARGETS dca DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  endif(NOT ${bredala_only})

endif(Boost_FOUND)
