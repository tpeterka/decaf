INCLUDE_DIRECTORIES(.)

find_package(Boost COMPONENTS serialization REQUIRED)

if(Boost_FOUND)

  include_directories(${Boost_INCLUDE_DIRS})
  ADD_LIBRARY(bca SHARED bredala.cpp)
  TARGET_LINK_LIBRARIES(bca ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  SET_TARGET_PROPERTIES(bca PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  INSTALL(TARGETS bca LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)  

  if(NOT ${bredala_only})
    ADD_LIBRARY(dca SHARED cdecaf.cpp)
    TARGET_LINK_LIBRARIES(dca ${Boost_LIBRARIES} bredala_transport bredala_datamodel bca)
    SET_TARGET_PROPERTIES(dca PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    INSTALL(TARGETS dca LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  endif(NOT ${bredala_only})

endif(Boost_FOUND)
