include (../../../cmake/UseCython.cmake)

cython_add_module           (py_lammps_workflow driver.pyx)
target_link_libraries       (py_lammps_workflow ${libraries})
