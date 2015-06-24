include (../../../cmake/UseCython.cmake)

cython_add_module           (py_lammps driver.pyx)
target_link_libraries       (py_lammps ${lammps_libraries})

install(TARGETS py_lammps
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
install(DIRECTORY .
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
        FILES_MATCHING PATTERN "*.py*"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    cython_add_module           (py_lammps_redist driver_redist.pyx)
    target_link_libraries       (py_lammps_redist ${lammps_libraries} ${Boost_LIBRARIES}  decaf_transport decaf_datamodel)

    install(TARGETS py_lammps_redist
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
endif(Boost_FOUND)
