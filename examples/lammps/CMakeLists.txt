set                         (lammps_libraries ${libraries} ${LAMMPS_LIBRARY} ${FFTW_LIBRARY})

find_path                   (LAMMPS_INCLUDE_DIR     lammps.h)
find_library                (LAMMPS_LIBRARY NAMES   lammps)

if                          (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)

  find_path                 (FFTW_INCLUDE_DIR       fftw.h)
  find_library              (FFTW_LIBRARY NAMES     fftw)
  if                        (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    set                     (lammps_libraries       ${lammps_libraries} )
    message                 (STATUS "Found LAMMPS; building LAMMPS example")
  else                      (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    message                 (STATUS "Need FFTW (not found) for LAMMPS; not building LAMMPS example")
  endif                     (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
else                        (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  message                   (STATUS "LAMMPS not found; not building LAMMPS example")

endif                       (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)

if                          (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)

  add_subdirectory          (melting)
  add_subdirectory          (freezing)

endif                       (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
