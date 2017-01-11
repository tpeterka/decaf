project                     (Decaf)
cmake_minimum_required      (VERSION 2.8)

option                      (transport_mpi      "Build Decaf with MPI transport layer"         ON)
option                      (tess_dense         "Build tessellation density estimator example" OFF)
option                      (gromacs_example    "Build Gromacs example"                        OFF)
option                      (gromacs_flowvr     "Build Gromacs example with FlowVR enable"     OFF)
option			    (bredala_only	"Build only Bredala libraries and exameples"   OFF)

set                         (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# OSX flags
if                          (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions	    (-DMAC_OSX)
    set			    (CMAKE_MACOSX_RPATH	    on)
endif                       ()

# C++11
set                         (CMAKE_CXX_STANDARD        11)
set                         (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# MPI
if                          (transport_mpi)
  find_package              (MPI REQUIRED)
  if                        (NOT bgq)
    set                     (transport_libraries        ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif                     ()
  add_definitions           (-DTRANSPORT_MPI)
endif                       (transport_mpi)

# Python
find_package                (PythonLibs)
if                          (PYTHONLIBS_FOUND)
  message                   (STATUS "Building Python examples.")
  set                       (python_examples             ON)
else                        ()
  message                   (STATUS "PythonLibs not found. Not building Python examples.")
  set                       (python_examples             OFF)
endif                       ()

# Boost
find_package                (Boost COMPONENTS serialization REQUIRED)

# Set include directories
set                         (CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem")
include_directories         (${Boost_INCLUDE_DIRS}
                             ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}/include
                             SYSTEM ${MPI_INCLUDE_PATH})

# Set libraries
set                         (libraries
                             ${libraries}
                             ${transport_libraries}
                             ${CMAKE_DL_LIBS})

# subdirectories
add_subdirectory            (src)
add_subdirectory            (examples)
message                     (STATUS "Examples configured")
if                          (PYTHONLIBS_FOUND)
  add_subdirectory          (python)
endif                       ()

# Install the headers
file(GLOB DEPLOY_FILES_AND_DIRS "${PROJECT_SOURCE_DIR}/include/*")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
   if( IS_DIRECTORY "${ITEM}" )
      list( APPEND DIRS_TO_DEPLOY "${ITEM}" )
   else()
      list( APPEND FILES_TO_DEPLOY "${ITEM}" )
   endif()
endforeach()
install( FILES ${FILES_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include )
install( DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include )

install ( FILES cmake/FindDecaf.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake )
