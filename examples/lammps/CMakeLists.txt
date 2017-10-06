# LAMMPS

set(lammps_libraries ${libraries} ${LAMMPS_LIBRARY} ${FFTW_LIBRARY})

find_path                   (LAMMPS_INCLUDE_DIR		lammps.h)
find_library                (LAMMPS_LIBRARY NAMES	lammps)
if                          (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  find_path                 (FFTW_INCLUDE_DIR		fftw.h)
  find_library              (FFTW_LIBRARY NAMES	        fftw)
  if                        (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    set                     (lammps_libraries ${lammps_libraries} )
    message                 (STATUS "Found LAMMPS; building LAMMPS example")
  else                      (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    message                 (STATUS "Need FFTW (not found) for LAMMPS; not building LAMMPS example")
  endif                     (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
else                        (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  message                   (STATUS "LAMMPS not found; not building LAMMPS example")
endif                       (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)

if(LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)

  include_directories       (${LAMMPS_INCLUDE_DIR} ${FFTW_INCLUDE_DIR})

  add_executable            (lammps            lammps.cpp)
  add_library               (mod_lammps MODULE lammps.cpp)
  target_link_libraries     (lammps
    ${lammps_libraries} ${bredala_transports} bredala_datamodel manala decaf)
  target_link_libraries   (mod_lammps
    ${lammps_libraries} ${bredala_transports} bredala_datamodel manala decaf)

  # .SO file extension on Linux/Mac OS
  set_target_properties(mod_lammps PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties(mod_lammps PROPERTIES PREFIX "")

  install(TARGETS lammps mod_lammps
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES  LAMMPS_TEST in.melt
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
