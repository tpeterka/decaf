include (../../../cmake/UseCython.cmake)

find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)

  message("Building cfd example with python")
  include_directories(${Boost_INCLUDE_DIRS})
  cython_add_module( py_cfd driver.pyx )
  target_link_libraries( py_cfd ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

  install(TARGETS py_cfd
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    driver-no-overlap.py
    driver-partial-overlap.py
    driver-total-overlap.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/python/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)

