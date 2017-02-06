message("Building contract example")

add_executable              (contract                contract.cpp)
add_library                 (mod_contract   MODULE   contract.cpp)

add_executable              (period_3nodes                period_3nodes.cpp)
add_library                 (mod_period_3nodes   MODULE   period_3nodes.cpp)

add_executable				(test_object			test_object.cpp)
add_library					(mod_test_object MODULE test_object.cpp)


target_link_libraries       (contract
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_contract
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (period_3nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_period_3nodes
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries		(test_object
${libraries} bredala_transport bredala_datamodel)
target_link_libraries		(mod_test_object
${libraries} bredala_transport bredala_datamodel)


# .SO file extension on Linux/Mac OS
set_target_properties(mod_contract  PROPERTIES SUFFIX ".so")
set_target_properties(mod_period_3nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_test_object PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_contract  PROPERTIES PREFIX "")
set_target_properties(mod_period_3nodes PROPERTIES PREFIX "")
set_target_properties(mod_test_object PROPERTIES PREFIX "")

install(TARGETS contract mod_contract
  period_3nodes mod_period_3nodes
  test_object mod_test_object
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES contract.py period_3nodes.py test_object.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
