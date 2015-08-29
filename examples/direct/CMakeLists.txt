Message("Decaf transport : ${decaf_transport}")

if                          (python_examples)
    add_subdirectory          (python)
endif                       (python_examples)

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)

  message("Building direct example")
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable              (linear_2nodes             linear_2nodes.cpp)
    add_executable              (linear_3nodes             linear_3nodes.cpp)
    add_library                 (mod_linear_2nodes  MODULE linear_2nodes.cpp)
    add_library                 (mod_linear_3nodes  MODULE linear_3nodes.cpp)
    target_link_libraries       (linear_2nodes
      ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (linear_3nodes
      ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (mod_linear_2nodes
      ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (mod_linear_3nodes
      ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    install(TARGETS linear_2nodes mod_linear_2nodes linear_3nodes mod_linear_3nodes
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_WRITE GROUP_EXECUTE
            WORLD_READ WORLD_WRITE WORLD_EXECUTE)

    install(FILES LINEAR_2NODES_TEST LINEAR_3NODES_TEST
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_WRITE GROUP_EXECUTE
            WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
