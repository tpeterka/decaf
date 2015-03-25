include (../../../UseCython.cmake)

cython_add_module           (py_lammps driver.pyx)
target_link_libraries       (py_lammps ${libraries})
