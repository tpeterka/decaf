include (../../../UseCython.cmake)

cython_add_module( py_lammps driver.pyx )
