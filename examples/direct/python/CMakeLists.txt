find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)
  add_definitions(-DPYBIND11)
  include_directories(${Boost_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})

  message("Building python direct example")
  add_library(py_linear_2nodes MODULE ../linear_2nodes.cpp)
  add_library(py_linear_3nodes MODULE ../linear_3nodes.cpp)
  add_library(py_cycle_4nodes  MODULE ../cycle_4nodes.cpp)

  target_link_libraries(py_linear_2nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries(py_linear_3nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
  target_link_libraries(py_cycle_4nodes
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

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
  set_target_properties(py_linear_2nodes PROPERTIES SUFFIX ".so")
  set_target_properties(py_linear_3nodes PROPERTIES SUFFIX ".so")
  set_target_properties(py_cycle_4nodes  PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(py_linear_2nodes PROPERTIES PREFIX "")
  set_target_properties(py_linear_3nodes PROPERTIES PREFIX "")
  set_target_properties(py_cycle_4nodes  PROPERTIES PREFIX "")

  # Strip unnecessary sections of the binary on Linux/Mac OS
  if(APPLE)
    set_target_properties(py_linear_2nodes PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_linear_3nodes PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_cycle_4nodes  PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_linear_2nodes PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    set_target_properties(py_linear_3nodes PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    set_target_properties(py_cycle_4nodes  PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET py_linear_2nodes
        POST_BUILD COMMAND strip -u -r py_linear_2nodes.so)
      add_custom_command(TARGET py_linear_3nodes
        POST_BUILD COMMAND strip -u -r py_linear_3nodes.so)
      add_custom_command(TARGET py_cycle_4nodes
        POST_BUILD COMMAND strip -u -r py_cycle_4nodes.so)
    endif()
  else(APPLE)
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET py_linear_2nodes
        POST_BUILD COMMAND strip py_linear_2nodes.so)
      add_custom_command(TARGET py_linear_3nodes
        POST_BUILD COMMAND strip py_linear_3nodes.so)
      add_custom_command(TARGET py_cycle_4nodes
        POST_BUILD COMMAND strip py_cycle_4nodes.so)
    endif()
  endif(APPLE)

  install(TARGETS
    py_linear_2nodes
    py_linear_3nodes
    py_cycle_4nodes
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    linear-2nodes-no-overlap.py
    linear-2nodes-partial-overlap.py
    linear-2nodes-total-overlap.py
    linear-3nodes-no-overlap.py
    linear-3nodes-partial-overlap.py
    linear-3nodes-total-overlap.py
    cycle-4nodes-no-overlap.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/direct/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)

