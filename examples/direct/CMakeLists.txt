add_executable              (direct            direct.cpp)
add_library                 (mod_direct MODULE direct.cpp)
target_link_libraries       (direct     ${libraries}     )
target_link_libraries       (mod_direct ${libraries}     )

if                          (python_examples)
  add_subdirectory          (python)
endif                       (python_examples)

install(TARGETS direct mod_direct
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install(FILES DIRECT_TEST
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
