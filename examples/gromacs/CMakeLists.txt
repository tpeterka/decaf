message("Building Gromacs example")
include(ExternalProject)

set(GROMACS_COMPILATION_OPTIONS -DGMX_MPI=ON -DGMX_CPU_ACCELERATION:STRING=SSE4.1 -DGMX_OPENMP:BOOL=TRUE -DGMX_THREAD_MPI=OFF -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}/examples/gromacs -DCMAKE_BUILD_TYPE:STRING=Release -DFFTWF_LIBRARY:PATH=${FFTW_PREFIX}/lib/libfftw3f.so)

ExternalProject_Add(gromacsmodify
    PREFIX "gromacs-decaf"
    URL ftp://ftp.gromacs.org/pub/gromacs/gromacs-4.6.7.tar.gz
    PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patch_gromacs.txt
    CMAKE_ARGS ${GROMACS_COMPILATION_OPTIONS}
)

add_executable              (prod_c_gromacs      prod_c.c)
add_executable              (treatment          treatment.cpp)
add_executable              (dflow_gromacs      dflow.cpp)
add_library                 (mod_dflow_gromacs  MODULE dflow.cpp)


target_link_libraries       (treatment
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
target_link_libraries       (dflow_gromacs
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
target_link_libraries       (mod_dflow_gromacs
    ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
target_link_libraries       (prod_c_gromacs bca dca)

  install(TARGETS treatment dflow_gromacs mod_dflow_gromacs prod_c_gromacs
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES GROMACS_TEST
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)
