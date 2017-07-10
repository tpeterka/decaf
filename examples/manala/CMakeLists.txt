
message("Building buffer example")

add_executable              (prod_buffer                prod.cpp)
add_executable              (filereader                 filereader.cpp)
add_executable              (dflow_buffer               dflow.cpp)
add_executable              (con_buffer                 con.cpp)
add_library                 (mod_dflow_buffer   MODULE  dflow.cpp)
add_executable              (simple_onesided            simple_onesided.cpp)

target_link_libraries       (prod_buffer 
${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (filereader
${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (dflow_buffer
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (con_buffer
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (mod_dflow_buffer
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (simple_onesided
  ${libraries} ${bredala_transports} bredala_datamodel manala decaf)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_dflow_buffer  PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_dflow_buffer  PROPERTIES PREFIX "")


install(TARGETS prod_buffer dflow_buffer
  con_buffer mod_dflow_buffer
  simple_onesided filereader
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/buffer/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES buffer-2nodes.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/buffer/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
