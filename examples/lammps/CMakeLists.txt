add_executable              (lammps            lammps.cpp)
add_library                 (mod_lammps MODULE lammps.cpp)
target_link_libraries       (lammps     ${libraries}     )

add_subdirectory            (python)
