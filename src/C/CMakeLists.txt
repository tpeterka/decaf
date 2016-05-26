INCLUDE_DIRECTORIES(.)

find_package(Boost COMPONENTS serialization REQUIRED)

if(Boost_FOUND)

  include_directories(${Boost_INCLUDE_DIRS})
  ADD_LIBRARY(bca SHARED bredala.cpp)
  TARGET_LINK_LIBRARIES(bca ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  SET_TARGET_PROPERTIES(bca PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  INSTALL(TARGETS bca LIBRARY DESTINATION lib)

  FILE(GLOB include-files ./*.h)
  INSTALL(FILES ${include-files} DESTINATION include/C/ PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )

endif(Boost_FOUND)
