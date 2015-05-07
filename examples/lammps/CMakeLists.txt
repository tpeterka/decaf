add_executable              (lammps-workflow     lammps.cpp)
target_link_libraries       (lammps-workflow     ${libraries})

add_subdirectory            (python)
