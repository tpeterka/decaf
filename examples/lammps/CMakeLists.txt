# LAMMPS

set(lammps_libraries ${libraries} ${LAMMPS_LIBRARY} ${FFTW_LIBRARY})

find_path                   (LAMMPS_INCLUDE_DIR		lammps.h)
find_library                (LAMMPS_LIBRARY NAMES	lammps)
if                          (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  find_path                 (FFTW_INCLUDE_DIR		fftw.h)
  find_library              (FFTW_LIBRARY NAMES	        fftw)
  if                        (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    set(lammps_libraries ${lammps_libraries} )
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
    target_link_libraries       (lammps     ${lammps_libraries} )
    target_link_libraries       (mod_lammps ${lammps_libraries} )



    install(TARGETS lammps mod_lammps
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
    install(FILES in.melt LAMMPS_TEST
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
    find_package(Boost COMPONENTS serialization)
    if(Boost_FOUND)
        add_executable              (lammps_redist            lammps_redist.cpp)
        add_library                 (mod_lammps_redist MODULE lammps_redist.cpp)
        target_link_libraries       (lammps_redist     ${lammps_libraries} ${Boost_LIBRARIES})
        target_link_libraries       (mod_lammps_redist ${lammps_libraries} ${Boost_LIBRARIES})

        install(TARGETS lammps_redist mod_lammps_redist
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
        install(FILES  LAMMPS_REDIST_TEST
                DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/lammps/
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
    endif(Boost_FOUND)
    if                          (python_examples)
      add_subdirectory          (python)
    endif                       (python_examples)
endif(LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY AND FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
