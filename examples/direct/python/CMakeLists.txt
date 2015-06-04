include (../../../cmake/UseCython.cmake)

cython_add_module( py_direct driver.pyx )
target_link_libraries( py_direct ${libraries})

install(TARGETS py_direct
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
install(DIRECTORY .
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
        FILES_MATCHING PATTERN "*.py*"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building direct example python with Redistribution component")
    cython_add_module( py_direct_redist driver_redist.pyx )
    target_link_libraries( py_direct_redist ${libraries} ${Boost_LIBRARIES})

    install(TARGETS py_direct_redist
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
endif(Boost_FOUND)

