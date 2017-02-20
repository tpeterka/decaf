if(NOT ${bredala_only} )

  add_subdirectory            (direct)
  add_subdirectory            (buffer)

  add_subdirectory            (flowvr)
  if                           (LAMMPS_INCLUDE_DIR AND
                               LAMMPS_LIBRARY     AND
                               FFTW_INCLUDE_DIR   AND
                               FFTW_LIBRARY)
    add_subdirectory          (lammps)
    message                   ("Building lammps example")
  else                        ()
    message                   ("Don't have LAMMPS_INCLUDE_DIR, LAMMPS_LIBRARY, FFTW_INCLUDE_DIR, and FFTW_LIBRARY; not building lammps example")
  endif                       ()

  if                          (tess_dense)
    add_subdirectory          (tess_dense)
  endif                       ()

  #add_subdirectory          (block_data_model)

  #add_subdirectory            (cfd)
endif(NOT ${bredala_only})

add_subdirectory            (C)

