find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})

  message("Building python direct mpmd example")
  add_library(py_linear_2nodes_prod  MODULE ../../linear_2nodes.cpp)
  add_library(py_linear_2nodes_dflow MODULE ../../linear_2nodes.cpp)
  add_library(py_linear_2nodes_con   MODULE ../../linear_2nodes.cpp)

  target_link_libraries(py_linear_2nodes_prod
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  target_link_libraries(py_linear_2nodes_dflow
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  target_link_libraries(py_linear_2nodes_con
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)

  # following copied from
  # http://pybind11.readthedocs.org/en/latest/cmake.html#cmake
  #
  # It's quite common to have multiple copies of the same Python version
  # installed on one's system; this will cause segfaults when multiple
  # conflicting Python instances are active at the same time (even when they
  # are of the same version). The solution for Linux and Mac OS is simple: we just don't
  # link against the Python library. The resulting shared library will have
  # missing symbols, but that's perfectly fine -- they will be resolved at
  # import time.

  # .SO file extension on Linux/Mac OS
  set_target_properties(py_linear_2nodes_prod  PROPERTIES SUFFIX ".so")
  set_target_properties(py_linear_2nodes_dflow PROPERTIES SUFFIX ".so")
  set_target_properties(py_linear_2nodes_con   PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(py_linear_2nodes_prod  PROPERTIES PREFIX "")
  set_target_properties(py_linear_2nodes_dflow PROPERTIES PREFIX "")
  set_target_properties(py_linear_2nodes_con   PROPERTIES PREFIX "")

  # Strip unnecessary sections of the binary on Linux/Mac OS
  if(APPLE)
    set_target_properties(py_linear_2nodes_prod  PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_linear_2nodes_dflow PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_linear_2nodes_con   PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_linear_2nodes_prod  PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    set_target_properties(py_linear_2nodes_dflow PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    set_target_properties(py_linear_2nodes_con   PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET py_linear_2nodes_prod
        POST_BUILD COMMAND strip -u -r py_linear_2nodes_prod.so)
      add_custom_command(TARGET py_linear_2nodes_dflow
        POST_BUILD COMMAND strip -u -r py_linear_2nodes_dflow.so)
      add_custom_command(TARGET py_linear_2nodes_con
        POST_BUILD COMMAND strip -u -r py_linear_2nodes_con.so)
    endif()
  else(APPLE)
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET py_linear_2nodes_prod
        POST_BUILD COMMAND strip py_linear_2nodes_prod.so)
      add_custom_command(TARGET py_linear_2nodes_dflow
        POST_BUILD COMMAND strip py_linear_2nodes_dflow.so)
      add_custom_command(TARGET py_linear_2nodes_con
        POST_BUILD COMMAND strip py_linear_2nodes_con.so)
    endif()
  endif(APPLE)

  install(TARGETS
    py_linear_2nodes_prod
    py_linear_2nodes_dflow
    py_linear_2nodes_con
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/mpmd/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    linear_2nodes_no_overlap.py
    linear_2nodes_prod.py
    linear_2nodes_dflow.py
    linear_2nodes_con.py
    DIRECT_TEST
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/mpmd/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)

