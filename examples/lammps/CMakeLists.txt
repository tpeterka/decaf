# LAMMPS
find_path                   (LAMMPS_INCLUDE_DIR		lammps.h)
find_library                (LAMMPS_LIBRARY NAMES	lammps)
if                          (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  find_path                 (FFTW_INCLUDE_DIR		fftw.h)
  find_library              (FFTW_LIBRARY NAMES	        fftw)
  if                        (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    message                 (STATUS "Found LAMMPS; building LAMMPS example")
  else                      (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    message                 (STATUS "Need FFTW (not found) for LAMMPS; not building LAMMPS example")
  endif                     (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
else                        (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  message                   (STATUS "LAMMPS not found; not building LAMMPS example")
endif                       (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)

if(LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    include_directories     (${LAMMPS_INCLUDE_DIR}
                             ${FFTW_INCLUDE_DIR})
    add_executable              (lammps            lammps.cpp)
    add_library                 (mod_lammps MODULE lammps.cpp)
    target_link_libraries       (lammps     ${libraries} ${LAMMPS_LIBRARY} ${FFTW_LIBRARY}  )
    target_link_libraries       (mod_lammps ${libraries} ${LAMMPS_LIBRARY} ${FFTW_LIBRARY}  )

    if                          (python_examples)
      add_subdirectory          (python)
    endif                       (python_examples)
endif(LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)

