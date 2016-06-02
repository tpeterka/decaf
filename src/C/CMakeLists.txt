INCLUDE_DIRECTORIES(.)

find_package(Boost COMPONENTS serialization REQUIRED)

if(Boost_FOUND)

  include_directories(${Boost_INCLUDE_DIRS})
  ADD_LIBRARY(bca SHARED bredala.cpp)
  TARGET_LINK_LIBRARIES(bca ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  SET_TARGET_PROPERTIES(bca PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  ADD_LIBRARY(dca SHARED cdecaf.cpp)
  TARGET_LINK_LIBRARIES(dca ${Boost_LIBRARIES} decaf_transport decaf_datamodel bca)
  SET_TARGET_PROPERTIES(dca PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  INSTALL(TARGETS bca dca LIBRARY DESTINATION lib)

endif(Boost_FOUND)
