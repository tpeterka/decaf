include (../../../cmake/UseCython.cmake)

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)

  cython_add_module           (py_lammps_redist driver_redist.pyx)
    target_link_libraries       (py_lammps_redist
      ${lammps_libraries} ${Boost_LIBRARIES}  decaf_transport decaf_datamodel)

    install(TARGETS py_lammps_redist
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_WRITE GROUP_EXECUTE
        WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    driver-redist-2nodes.py
    driver-redist-3nodes-2cons.py
    driver-redist-4nodes.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
