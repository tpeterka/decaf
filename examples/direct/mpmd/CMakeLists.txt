if                          (python_examples)
  add_subdirectory          (python)
endif                       (python_examples)

# find_package                (Boost COMPONENTS serialization REQUIRED)

# if(Boost_FOUND)

  message("Building direct mpmd example")
  # include_directories(${Boost_INCLUDE_DIRS})

  add_executable              (prod               prod.cpp)
  add_executable              (con                con.cpp)
  add_executable              (dflow              dflow.cpp)
  add_library                 (mod_dflow          MODULE dflow.cpp)

  target_link_libraries       (prod
    ${libraries} bredala_transport bredala_datamodel)
  target_link_libraries       (con
    ${libraries} bredala_transport bredala_datamodel)
  target_link_libraries       (dflow
    ${libraries} bredala_transport bredala_datamodel)
  target_link_libraries       (mod_dflow
    ${libraries} bredala_transport bredala_datamodel)

  # .SO file extension on Linux/Mac OS
  set_target_properties(mod_dflow PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(mod_dflow PROPERTIES PREFIX "")

  install(TARGETS prod con dflow mod_dflow
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/mpmd/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES LINEAR_2NODES_TEST linear-2nodes.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/mpmd/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

# endif(Boost_FOUND)
