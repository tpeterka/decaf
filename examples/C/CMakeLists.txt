find_package(Boost COMPONENTS serialization REQUIRED)
if(Boost_FOUND)

  message("Building C examples")
  include_directories(${Boost_INCLUDE_DIRS})

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra")

  ADD_EXECUTABLE(SimpleBCA simplebca.c)

  TARGET_LINK_LIBRARIES(SimpleBCA bca)

  ADD_EXECUTABLE(RedistBCA redistbca.c)

  TARGET_LINK_LIBRARIES(RedistBCA bca)

  INSTALL(TARGETS SimpleBCA  RedistBCA
          DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/C/)
  
  INSTALL(FILES REDIST_TEST 
	  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/C/
    	  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    	  GROUP_READ GROUP_WRITE GROUP_EXECUTE
    	  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  if( NOT ${bredala_only} )
    add_executable(linear_2nodes_c linear_2nodes_c.c)
    add_executable(linear_3nodes_c linear_3nodes_c.c)
    add_executable(cycle_4nodes_c cycle_4nodes_c.c)
    add_library(mod_dflow_direct  MODULE dflow.cpp)

    target_link_libraries(linear_2nodes_c bca dca)
    target_link_libraries(linear_3nodes_c bca dca)
    target_link_libraries(cycle_4nodes_c bca dca)
    target_link_libraries(mod_dflow_direct
      ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)

    INSTALL(TARGETS linear_2nodes_c
      linear_3nodes_c mod_dflow_direct
      cycle_4nodes_c
      DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/C/)

    INSTALL(FILES REDIST_TEST LINEAR_2NODES_TEST LINEAR_3NODES_TEST CYCLE_4NODES_TEST
      DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/C/
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_WRITE GROUP_EXECUTE
      WORLD_READ WORLD_WRITE WORLD_EXECUTE)

    add_subdirectory(mpmd)
  endif(NOT  ${bredala_only})
endif(Boost_FOUND)

