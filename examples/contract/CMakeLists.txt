message("Building contract example")

add_executable              (contract                contract.cpp)
add_library                 (mod_contract   MODULE   contract.cpp)

target_link_libraries       (contract
${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_contract
  ${libraries} bredala_transport bredala_datamodel)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_contract  PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_contract  PROPERTIES PREFIX "")


install(TARGETS contract mod_contract
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES contract.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
