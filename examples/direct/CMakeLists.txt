Message("Decaf transport : ${decaf_transport}")

if                          (python_examples)
    add_subdirectory          (python)
endif                       (python_examples)

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building direct example with Redistribution component")
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable              (direct_redist             direct_redist.cpp)
    add_executable              (linear_3_nodes            linear_3_nodes.cpp)
    add_library                 (mod_direct_redist  MODULE direct_redist.cpp)
    add_library                 (mod_linear_3_nodes MODULE linear_3_nodes.cpp)
    target_link_libraries       (direct_redist      ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (linear_3_nodes     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (mod_direct_redist  ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (mod_linear_3_nodes ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    install(TARGETS linear_3_nodes mod_linear_3_nodes direct_redist mod_direct_redist
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

    install(FILES DIRECT_REDIST_TEST LINEAR_3_NODES_TEST
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
