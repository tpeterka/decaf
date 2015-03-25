add_executable              (lammps lammps.cpp)
target_link_libraries       (lammps     ${libraries})

add_subdirectory            (python)
