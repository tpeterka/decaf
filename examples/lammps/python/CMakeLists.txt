include (../../../cmake/UseCython.cmake)

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)

  cython_add_module           (py_lammps driver.pyx)
    target_link_libraries       (py_lammps
      ${lammps_libraries} ${Boost_LIBRARIES}  decaf_transport decaf_datamodel)

    install(TARGETS py_lammps
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_WRITE GROUP_EXECUTE
        WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    driver-2nodes.py
    driver-3nodes-2cons.py
    driver-4nodes.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
