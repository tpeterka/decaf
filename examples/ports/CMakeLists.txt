message("Building ports example")

add_executable              (ports                ports.cpp)
add_library                 (mod_ports   MODULE   ports.cpp)

target_link_libraries       (ports
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_ports
  ${libraries} bredala_transport bredala_datamodel)


# .SO file extension on Linux/Mac OS
set_target_properties(mod_ports  PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_ports  PROPERTIES PREFIX "")

install(TARGETS ports mod_ports
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/ports/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES ports.py 
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/ports/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
