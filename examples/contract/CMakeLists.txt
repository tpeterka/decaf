#add_subdirectory            (mini_tests)

message("Building contract example")

add_executable              (contract                contract.cpp)
add_library                 (mod_contract   MODULE   contract.cpp)

add_executable              (contract_ports                contract_ports.cpp)
add_library                 (mod_contract_ports   MODULE   contract_ports.cpp)

add_executable              (period_3nodes                period_3nodes.cpp)
add_library                 (mod_period_3nodes   MODULE   period_3nodes.cpp)

add_executable              (simple_2nodes                simple_2nodes.cpp)
add_library                 (mod_simple_2nodes   MODULE   simple_2nodes.cpp)


target_link_libraries       (contract
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_contract
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (contract_ports
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_contract_ports
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (period_3nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_period_3nodes
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (simple_2nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_simple_2nodes
  ${libraries} bredala_transport bredala_datamodel)


# .SO file extension on Linux/Mac OS
set_target_properties(mod_contract  PROPERTIES SUFFIX ".so")
set_target_properties(mod_contract_ports  PROPERTIES SUFFIX ".so")
set_target_properties(mod_period_3nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_simple_2nodes PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_contract  PROPERTIES PREFIX "")
set_target_properties(mod_contract_ports  PROPERTIES PREFIX "")
set_target_properties(mod_period_3nodes PROPERTIES PREFIX "")
set_target_properties(mod_simple_2nodes PROPERTIES PREFIX "")

install(TARGETS contract mod_contract
  contract_ports mod_contract_ports
  period_3nodes mod_period_3nodes
  simple_2nodes mod_simple_2nodes
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES contract.py contract_ports.py
  period_3nodes.py simple_2nodes.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
