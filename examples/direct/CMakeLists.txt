if                          (python_examples)
  add_subdirectory          (python)
endif                       (python_examples)

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)

  message("Building direct example")
  include_directories(${Boost_INCLUDE_DIRS})

  add_executable              (linear_2nodes             linear_2nodes.cpp)
  add_executable              (linear_3nodes             linear_3nodes.cpp)
  add_executable              (cycle_4nodes              cycle_4nodes.cpp)
  add_library                 (mod_linear_2nodes  MODULE linear_2nodes.cpp)
  add_library                 (mod_linear_3nodes  MODULE linear_3nodes.cpp)
  add_library                 (mod_cycle_4nodes   MODULE cycle_4nodes.cpp)

  target_link_libraries       (linear_2nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries       (linear_3nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries       (cycle_4nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries       (mod_linear_2nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries       (mod_linear_3nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries       (mod_cycle_4nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

  # .SO file extension on Linux/Mac OS
  set_target_properties(mod_linear_2nodes PROPERTIES SUFFIX ".so")
  set_target_properties(mod_linear_3nodes PROPERTIES SUFFIX ".so")
  set_target_properties(mod_cycle_4nodes  PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(mod_linear_2nodes PROPERTIES PREFIX "")
  set_target_properties(mod_linear_3nodes PROPERTIES PREFIX "")
  set_target_properties(mod_cycle_4nodes  PROPERTIES PREFIX "")


  install(TARGETS linear_2nodes mod_linear_2nodes
    linear_3nodes mod_linear_3nodes
    cycle_4nodes mod_cycle_4nodes
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES LINEAR_2NODES_TEST LINEAR_3NODES_TEST CYCLE_4NODES_TEST
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
