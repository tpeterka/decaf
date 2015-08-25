include (../../../cmake/UseCython.cmake)

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building direct example python with Redistribution component")
    include_directories(${Boost_INCLUDE_DIRS})
    cython_add_module(py_redist driver_redist.pyx )
    target_link_libraries( py_redist ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    install(TARGETS py_redist
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
endif(Boost_FOUND)

