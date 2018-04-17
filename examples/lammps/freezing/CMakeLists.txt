include_directories         (${LAMMPS_INCLUDE_DIR} ${FFTW_INCLUDE_DIR})

add_executable              (freeze            lammps.cpp)
target_link_libraries       (freeze
                            ${lammps_libraries} ${bredala_transports} bredala_datamodel manala decaf)

add_executable              (detect            detect.cpp)
target_link_libraries       (detect
                            ${lammps_libraries} ${bredala_transports} bredala_datamodel manala decaf)

install                     (TARGETS freeze detect
                            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/freezing
                            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_WRITE GROUP_EXECUTE
                            WORLD_READ WORLD_WRITE WORLD_EXECUTE)

install                     (FILES  AW.tersoff data.init in.watbox lammps-2nodes.py
                            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/freezing
                            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_WRITE GROUP_EXECUTE
                            WORLD_READ WORLD_WRITE WORLD_EXECUTE)
