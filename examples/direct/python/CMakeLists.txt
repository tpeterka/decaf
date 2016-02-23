include (../../../cmake/UseCython.cmake)

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)

  include_directories(${Boost_INCLUDE_DIRS})

  cython_add_module(py_linear_2nodes driver_linear_2nodes.pyx)
  target_link_libraries(py_linear_2nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

  cython_add_module(py_linear_3nodes driver_linear_3nodes.pyx)
  target_link_libraries(py_linear_3nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

  # cython_add_module(py_cycle_4nodes driver_cycle_4nodes.pyx)
  # target_link_libraries(py_cycle_4nodes
  #   ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

  install(TARGETS py_linear_2nodes
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(TARGETS py_linear_3nodes
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  # install(TARGETS py_cycle_4nodes
  #   DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
  #   PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  #   GROUP_READ GROUP_WRITE GROUP_EXECUTE
  #   WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    driver-linear-2nodes-no-overlap.py
    driver-linear-2nodes-partial-overlap.py
    driver-linear-2nodes-total-overlap.py
    driver-linear-3nodes-no-overlap.py
    driver-linear-3nodes-partial-overlap.py
    driver-linear-3nodes-total-overlap.py
    # driver-cycle-4nodes-no-overlap.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)

