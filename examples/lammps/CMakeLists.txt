add_executable              (lammps            lammps.cpp)
add_library                 (mod_lammps MODULE lammps.cpp)
target_link_libraries       (lammps     ${libraries}     )
target_link_libraries       (mod_lammps ${libraries}     )

if                          (python_examples)
  add_subdirectory          (python)
endif                       (python_examples)

