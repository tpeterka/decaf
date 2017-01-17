message("Building Gromacs example")
include(ExternalProject)

# Damaris configuration library
find_package            (OpenMP REQUIRED)
if(OPENMP_FOUND)
    set                       (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set                       (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    message          ("OpenMP not found")
endif()

find_package(Damaris REQUIRED)
include_directories(${DAMARIS_INCLUDE_DIR})
list(APPEND DEP_LIBRARIES ${DAMARIS_LIBRARY})

add_definitions(-DPARALLEL)
list(APPEND DEP_LIBRARIES "stdc++" "rt" "dl")
find_package(Boost COMPONENTS filesystem system date_time serialization)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
list(APPEND DEP_LIBRARIES ${Boost_LIBRARIES})
find_package(XercesC REQUIRED)
include_directories(${XERCESC_INCLUDE})
list(APPEND DEP_LIBRARIES ${XERCESC_LIBRARY})
find_package(VisIt)
if(VisIt_FOUND)
    include_directories(${VisIt_INCLUDE_DIRS})
    message(STATUS "VisIt found: ${VisIt_INCLUDE_DIRS}")
    list(APPEND DEP_LIBRARIES ${VisIt_LIBRARIES})
else(VisIt_FOUND)
    message(STATUS "VisIt not found")
endif(VisIt_FOUND)

set(bin_suffix "")

if(${gromacs_flowvr})
    find_package(FlowVR REQUIRED)
    add_definitions(-DFLOWVR)
    add_subdirectory(flowvr)
    include_directories(${FLOWVR_INCLUDE_DIR})
    message(STATUS "FlowVR found: ${FLOWVR_INCLUDE_DIR}")
    list(APPEND DEP_LIBRARIES ${FLOWVR_base_LIBRARY} ${FLOWVR_mod_LIBRARY})
    set(bin_suffix "${bin_suffix}_flowvr")
endif(${gromacs_flowvr})



message(STATUS "Module path : ${CMAKE_MODULE_PATH}")

set(GROMACS_COMPILATION_OPTIONS -DCMAKE_PREFIX_PATH=${CMAKE_MODULE_PATH} -DGMX_MPI=ON -DGMX_CPU_ACCELERATION:STRING=SSE4.1 -DGMX_OPENMP:BOOL=TRUE -DGMX_THREAD_MPI=OFF -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}/examples/gromacs -DCMAKE_BUILD_TYPE:STRING=Release -DFFTWF_LIBRARY:PATH=${FFTW_PREFIX}/lib/libfftw3f.so)

ExternalProject_Add(gromacsmodify
    PREFIX "gromacs-decaf"
    URL ftp://ftp.gromacs.org/pub/gromacs/gromacs-4.6.7.tar.gz
    PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patch_gromacs.txt
    CMAKE_ARGS ${GROMACS_COMPILATION_OPTIONS}
)

set(GROMACS_COMPILATION_OPTIONS -DCMAKE_PREFIX_PATH=${CMAKE_MODULE_PATH} -DGMX_MPI=ON -DGMX_CPU_ACCELERATION:STRING=SSE -DGMX_THREAD_MPI=OFF -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}/examples/gromacs -DCMAKE_BUILD_TYPE:STRING=Release -DFFTWF_LIBRARY:PATH=${FFTW_PREFIX}/lib/libfftw3f.so)


ExternalProject_Add(gromacs-4.5.5modify
    PREFIX "gromacs-4.5.5-decaf"
    URL ftp://ftp.gromacs.org/pub/gromacs/gromacs-4.5.5.tar.gz
    PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/patch_gromacs_4.5.5.txt
    CMAKE_ARGS ${GROMACS_COMPILATION_OPTIONS}
)

add_executable              (prod_c_gromacs     prod_c.c)
add_executable              (treatment          treatment.cpp)
add_executable              (dflow_gromacs      dflow.cpp)
add_executable              (targetmanager      targetmanager.cpp)
add_library                 (mod_dflow_gromacs  MODULE dflow.cpp)


target_link_libraries       (treatment
    ${libraries} bredala_transport bredala_datamodel ${DEP_LIBRARIES})
target_link_libraries       (dflow_gromacs
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
target_link_libraries       (targetmanager
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel ${DEP_LIBRARIES})
target_link_libraries       (mod_dflow_gromacs
    ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
target_link_libraries       (prod_c_gromacs bca dca)

set_target_properties (treatment PROPERTIES LINK_FLAGS "-Wl,-export-dynamic")
set_target_properties (targetmanager PROPERTIES OUTPUT_NAME "targetmanager${bin_suffix}")

  install(TARGETS treatment dflow_gromacs mod_dflow_gromacs prod_c_gromacs targetmanager
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install(FILES GROMACS_TEST FEPA_TEST gromacs_workflow.py decaf_grid.xml gromacs_workflow_no_link.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_WRITE GROUP_EXECUTE
    WORLD_READ WORLD_WRITE WORLD_EXECUTE)
