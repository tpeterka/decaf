message("Building contract example")

add_executable              (contract                contract.cpp)
add_library                 (mod_contract   MODULE   contract.cpp)

add_executable				      (putget_index			         putget_index.cpp)
add_library					        (mod_putget_index		MODULE putget_index.cpp)

add_executable              (period_3nodes                period_3nodes.cpp)
add_library                 (mod_period_3nodes   MODULE   period_3nodes.cpp)

add_executable              (simple_2nodes                simple_2nodes.cpp)
add_library                 (mod_simple_2nodes   MODULE   simple_2nodes.cpp)

add_executable              (errors                errors.cpp)
add_library                 (mod_errors   MODULE   errors.cpp)

target_link_libraries       (contract
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_contract
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries		(putget_index
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries		(mod_putget_index
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (period_3nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_period_3nodes
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (simple_2nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_simple_2nodes
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (errors
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_errors
  ${libraries} bredala_transport bredala_datamodel)


# .SO file extension on Linux/Mac OS
set_target_properties(mod_contract  PROPERTIES SUFFIX ".so")
set_target_properties(mod_putget_index  PROPERTIES SUFFIX ".so")
set_target_properties(mod_period_3nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_simple_2nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_errors PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_contract  PROPERTIES PREFIX "")
set_target_properties(mod_putget_index PROPERTIES PREFIX "")
set_target_properties(mod_period_3nodes PROPERTIES PREFIX "")
set_target_properties(mod_simple_2nodes PROPERTIES PREFIX "")
set_target_properties(mod_errors PROPERTIES PREFIX "")

install(TARGETS contract mod_contract
  putget_index mod_putget_index
  period_3nodes mod_period_3nodes
  simple_2nodes mod_simple_2nodes
  errors mod_errors
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES contract.py putget_index.py
  period_3nodes.py simple_2nodes.py
  errors.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
