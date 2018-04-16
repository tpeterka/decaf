include_directories         (${LAMMPS_INCLUDE_DIR} ${FFTW_INCLUDE_DIR})

add_executable              (lammps            lammps.cpp)
add_library                 (mod_lammps MODULE lammps.cpp)
target_link_libraries       (lammps
                            ${lammps_libraries} ${bredala_transports} bredala_datamodel manala decaf)
target_link_libraries       (mod_lammps
                            ${lammps_libraries} ${bredala_transports} bredala_datamodel manala decaf)

# .SO file extension on Linux/Mac OS
set_target_properties(mod_lammps PROPERTIES SUFFIX ".so")

# Don't add a 'lib' prefix to the shared library
set_target_properties(mod_lammps PROPERTIES PREFIX "")

install                     (TARGETS lammps mod_lammps
                            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/melting/
                            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_WRITE GROUP_EXECUTE
                            WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install                     (FILES  in.melt lammps-4nodes.py
                            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/melting/
                            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_WRITE GROUP_EXECUTE
                            WORLD_READ WORLD_WRITE WORLD_EXECUTE)
