add_subdirectory            (direct)

#add_subdirectory            (datatypes)

add_subdirectory            (redist)

if                          (DLAMMPS_INCLUDE_DIR AND
                             DLAMMPS_LIBRARY     AND
                             DFFTW_INCLUDE_DIR   AND
                             DFFTW_LIBRARY)
  add_subdirectory          (lammps)
  message                   ("Building lammps example")
else                        ()
  message                   ("Don't have LAMMPS_INCLUDE_DIR, LAMMPS_LIBRARY, FFTW_INCLUDE_DIR, and FFTW_LIBRARY; not building lammps example")
endif                       ()

if                          (DIY_INCLUDE_DIRS  AND
                             TESS_INCLUDE_DIRS AND
                             TESS_LIBRARY)
  add_subdirectory          (block_data_model)
  message                   ("Building block_data_model example")
else                        ()
  message                   ("Don't have DIY_INCLUDE_DIRS, TESS_INCLUDE_DIRS, and TESS_LIBRARY; not building block_data_model example")
endif                       ()

add_subdirectory            (cfd)
