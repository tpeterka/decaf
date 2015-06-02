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

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building direct example with Redistribution component")
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable              (direct_redist            direct_redist.cpp)
    add_library                 (mod_direct_redist MODULE direct_redist.cpp)
    target_link_libraries       (direct_redist     ${libraries} ${Boost_LIBRARIES})
    target_link_libraries       (mod_direct_redist ${libraries} ${Boost_LIBRARIES})

    install(TARGETS direct_redist mod_direct_redist
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

    install(FILES DIRECT_REDIST_TEST
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
