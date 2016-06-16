project                     (Decaf)
cmake_minimum_required      (VERSION 2.8)

option                      (transport_mpi      "Build Decaf with MPI transport layer"         ON)
option                      (hacc               "Build HACC example"                           OFF)
option                      (gromacs            "Build Gromacs example"                        OFF)

set                         (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# OSX flags
if                          (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions	    (-DMAC_OSX)
    set			    (CMAKE_MACOSX_RPATH	    on)
endif                       ()

# C++11
set                         (CMAKE_CXX_STANDARD        11)
if                          ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
                             "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") #Syntax for CMAKE 3.+
  set                       (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
  set                       (CMAKE_EXE_LINKER_FLAGS     "-lc++")
  set                       (CMAKE_MODULE_LINKER_FLAGS  "-lc++")
  set                       (CMAKE_SHARED_LINKER_FLAGS  "-lc++")
else                        ()
  set                       (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif                       ()

# MPI
if                          (transport_mpi)
  find_package              (MPI REQUIRED)
  set                       (transport_libraries        ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  add_definitions           (-DTRANSPORT_MPI)
endif                       (transport_mpi)

# Threads
find_package		    (Threads)
find_package		    (OpenMP)
if 			    (OPENMP_FOUND)
  set                       (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set                       (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else			    ()
  message		    ("OpenMP not found")
  add_definitions	    (-DDECAF_NO_OPENMP)
endif                       ()
if                          (NOT diy_thread)
    message                 ("Diy threading is disabled; setting diy threads will have no effect")
    add_definitions         (-DDIY_NO_THREADS)
endif                       (NOT diy_thread)

# Python
find_package                (PythonLibs)
if                          (PYBIND11_INCLUDE_DIR AND PYTHONLIBS_FOUND)
  message                   (STATUS "Found PythonLibs and user provided PYBIND11_INCLUDE_DIR")
  message                   (STATUS "Building python examples")
  set                       (python_examples             ON)
else                        ()
  if                        (PYTHONLIBS_FOUND)
    message                 (STATUS "PYBIND11_INCLUDE_DIR must be manually provided")
  else                      ()
    message                 (STATUS "PythonLibs not found")
  endif                     ()
  message                   (STATUS "Not building python examples")
  set                       (python_examples             OFF)
endif                       ()

# Set include directories
set                         (CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem")
include_directories         (${CMAKE_CURRENT_BINARY_DIR}
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
