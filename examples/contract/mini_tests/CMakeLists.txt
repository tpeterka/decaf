message("Building mini_tests example")

add_executable              (mini1                mini1.cpp)
add_library                 (mod_mini1   MODULE   mini1.cpp)

add_executable              (mini2                mini2.cpp)
add_library                 (mod_mini2   MODULE   mini2.cpp)

add_executable              (mini3                mini3.cpp)
add_library                 (mod_mini3   MODULE   mini3.cpp)

add_executable				(test_object			test_object.cpp)
add_library       			(mod_test_object MODULE test_object.cpp)


target_link_libraries       (mini1
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_mini1
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (mini2
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_mini2
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries       (mini3
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_mini3
  ${libraries} bredala_transport bredala_datamodel)

target_link_libraries		(test_object
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries		(mod_test_object
  ${libraries} bredala_transport bredala_datamodel)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_mini1  PROPERTIES SUFFIX ".so")
set_target_properties(mod_mini2  PROPERTIES SUFFIX ".so")
set_target_properties(mod_mini3  PROPERTIES SUFFIX ".so")
set_target_properties(mod_test_object PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_mini1  PROPERTIES PREFIX "")
set_target_properties(mod_mini2  PROPERTIES PREFIX "")
set_target_properties(mod_mini3  PROPERTIES PREFIX "")
set_target_properties(mod_test_object PROPERTIES PREFIX "")

install(TARGETS mini1 mod_mini1
  mini2 mod_mini2 mini3 mod_mini3
  test_object mod_test_object
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/mini_tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES mini1.py 
  mini2.py mini3.py
  test_object.py
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/contract/mini_tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)
