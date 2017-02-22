message("Building small_tests example")

add_executable              (contract_link                contract_link.cpp)
add_library                 (mod_contract_link   MODULE   contract_link.cpp)

target_link_libraries       (contract_link
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_contract_link
  ${libraries} bredala_transport bredala_datamodel)


# .SO file extension on Linux/Mac OS
set_target_properties(mod_contract_link  PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_contract_link  PROPERTIES PREFIX "")

install(TARGETS contract_link mod_contract_link
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/small_tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES contract_link.py 
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/small_tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
