find_package(Boost COMPONENTS serialization)

if(Boost_FOUND)
  add_definitions(-DPYBIND11)
  include_directories(${Boost_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})

  message("Building python lammps example")
  add_library(py_lammps MODULE ../lammps.cpp)

  target_link_libraries(py_lammps
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
  set_target_properties(py_lammps PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(py_lammps PROPERTIES PREFIX "")

  # Strip unnecessary sections of the binary on Linux/Mac OS
  if(APPLE)
    set_target_properties(py_lammps PROPERTIES MACOSX_RPATH ".")
    set_target_properties(py_lammps PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET py_lammps
        POST_BUILD COMMAND strip -u -r py_lammps.so)
    endif()
  else(APPLE)
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command(TARGET py_lammps
        POST_BUILD COMMAND strip py_lammps.so)
    endif()
  endif(APPLE)

  install(TARGETS
    py_lammps
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES
    lammps-2nodes.py
    lammps-3nodes-2cons.py
    lammps-4nodes.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
