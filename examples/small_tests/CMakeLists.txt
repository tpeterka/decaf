message("Building small_tests example")

add_executable              (simple_2nodes                simple_2nodes.cpp)
add_library                 (mod_simple_2nodes   MODULE   simple_2nodes.cpp)

add_executable              (conversion					 conversion.cpp)
add_library                 (mod_conversion		MODULE   conversion.cpp)

add_executable              (period_link                period_link.cpp)
add_library                 (mod_period_link   MODULE   period_link.cpp)


target_link_libraries       (simple_2nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_simple_2nodes
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (conversion
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_conversion
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (period_link
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_period_link
  ${libraries} bredala_transport bredala_datamodel)


# .SO file extension on Linux/Mac OS
set_target_properties(mod_simple_2nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_conversion  PROPERTIES SUFFIX ".so")
set_target_properties(mod_period_link  PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_simple_2nodes PROPERTIES PREFIX "")
set_target_properties(mod_conversion  PROPERTIES PREFIX "")
set_target_properties(mod_period_link  PROPERTIES PREFIX "")

install(TARGETS simple_2nodes mod_simple_2nodes
  conversion mod_conversion
  period_link mod_period_link
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/small_tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES period_link.py conversion.py
  simple_2nodes.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/small_tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
