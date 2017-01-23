add_subdirectory            (mpmd)

message("Building direct example")

add_executable              (linear_2nodes             linear_2nodes.cpp)
add_executable              (linear_3nodes             linear_3nodes.cpp)
add_executable              (cycle_4nodes              cycle_4nodes.cpp)
add_executable				(contract						contract.cpp)
add_library					(mod_contract			MODULE contract.cpp)
add_library                 (mod_linear_2nodes  MODULE linear_2nodes.cpp)
add_library                 (mod_linear_3nodes  MODULE linear_3nodes.cpp)
add_library                 (mod_cycle_4nodes   MODULE cycle_4nodes.cpp)

target_link_libraries		(contract
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries		(mod_contract
  ${libraries} bredala_transport bredala_datamodel)


target_link_libraries       (linear_2nodes 
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (linear_3nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (cycle_4nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_linear_2nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_linear_3nodes
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_cycle_4nodes
  ${libraries} bredala_transport bredala_datamodel)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_linear_2nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_linear_3nodes PROPERTIES SUFFIX ".so")
set_target_properties(mod_cycle_4nodes  PROPERTIES SUFFIX ".so")
set_target_properties(mod_contract			PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_linear_2nodes PROPERTIES PREFIX "")
set_target_properties(mod_linear_3nodes PROPERTIES PREFIX "")
set_target_properties(mod_cycle_4nodes  PROPERTIES PREFIX "")
set_target_properties(mod_contract			PROPERTIES PREFIX "")

install(TARGETS linear_2nodes mod_linear_2nodes
  linear_3nodes mod_linear_3nodes
  cycle_4nodes mod_cycle_4nodes
  contract mod_contract
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES LINEAR_2NODES_TEST LINEAR_3NODES_TEST CYCLE_4NODES_TEST
  linear-2nodes.py linear-3nodes.py cycle-4nodes.py contract.py
  linear-2nodes-topo1.py linear-2nodes-topo2.py linear-2nodes-topo3.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
