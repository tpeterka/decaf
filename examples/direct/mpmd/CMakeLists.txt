message("Building direct mpmd example")

add_executable              (prod               prod.cpp)
add_executable              (con                con.cpp)
add_executable              (dflow              dflow.cpp)
add_library                 (mod_dflow          MODULE dflow.cpp)

target_link_libraries       (prod
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (con
    ${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (dflow
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (mod_dflow
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_dflow PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_dflow PROPERTIES PREFIX "")

install(TARGETS prod con dflow mod_dflow
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/mpmd/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES linear-2nodes.py bgq_run.py bgq_mapfile
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/mpmd/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
