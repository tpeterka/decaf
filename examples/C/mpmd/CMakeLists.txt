
find_package(Boost COMPONENTS serialization REQUIRED)

if(Boost_FOUND)

  message("Building direct mpmd example")
  include_directories(${Boost_INCLUDE_DIRS})

  add_executable              (prod_c               prod_c.c)
  add_executable              (con_c                con.cpp)
  add_executable              (dflow_c              dflow.cpp)
  add_library                 (mod_dflow_c          MODULE dflow.cpp)

  target_link_libraries       (prod_c
    ${libraries} ${Boost_LIBRARIES} bca dca)
  target_link_libraries       (con_c
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  target_link_libraries       (dflow_c
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  target_link_libraries       (mod_dflow_c
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)

  # .SO file extension on Linux/Mac OS
  set_target_properties(mod_dflow_c PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(mod_dflow_c PROPERTIES PREFIX "")

  install(TARGETS prod_c con_c dflow_c mod_dflow_c
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/C/mpmd/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES LINEAR_2NODES_TEST
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/C/mpmd/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
