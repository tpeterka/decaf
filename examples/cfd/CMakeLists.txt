message("Building cfd example")
add_executable              (cfd     cfd.cpp cfd_kernel.cpp)
add_library                 (mod_cfd MODULE cfd.cpp cfd_kernel.cpp)
target_link_libraries       (cfd
  ${libraries} bredala_transport bredala_datamodel)
target_link_libraries       (mod_cfd
  ${libraries} bredala_transport bredala_datamodel)

install(TARGETS cfd mod_cfd
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES CFD_TEST
  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_WRITE GROUP_EXECUTE
  WORLD_READ WORLD_WRITE WORLD_EXECUTE)

