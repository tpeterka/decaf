project                     (Decaf)
cmake_minimum_required      (VERSION 2.8)

option                      (debug              "Build Decaf with debugging on"                ON)
option                      (optimize           "Build Decaf with optimization"                OFF)
option                      (transport_mpi      "Build Decaf with MPI transport layer"         ON)
option                      (CYTHON_PREFIX      "Path to Cython directory"                       )

set                         (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

if                          (transport_mpi)
  find_package              (MPI REQUIRED)
  set                       (transport_libraries        ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  add_definitions           (-DTRANSPORT_MPI)
endif                       (transport_mpi)

# Debugging
if                          (debug)
  if                        (optimize)
    set                     (cxx_flags                  ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  else                      (optimize)
    set                     (cxx_flags                  ${CMAKE_CXX_FLAGS_DEBUG})
  endif                     (optimize)
  add_definitions           (-DDEBUG)
else                        (debug)
  if                        (optimize)
    set                     (cxx_flags                  ${CMAKE_CXX_FLAGS_RELEASE})
  else                      (optimize)
    set                     (cxx_flags                  ${CMAKE_CXX_FLAGS})
  endif                     (optimize)
endif                       (debug)
add_definitions             (${cxx_flags})

# Python
find_package                (Cython)
find_package                (PythonLibs)
if                          (Cython_FOUND AND PYTHONLIBS_FOUND)
    message                 (STATUS "Found Cython and PythonLibs; building python examples")
    set                     (python_examples             ON)
else                        (Cython_FOUND AND PYTHONLIBS_FOUND)
  if                        (Cython_FOUND)
    message                 (STATUS "PythonLibs not found; not building python examples")
  else                      (Cython_FOUND)
    message                 (STATUS "Cython not found; not building python examples")
  endif                     (Cython_FOUND)
    set                     (python_examples             OFF)
endif                       (Cython_FOUND AND PYTHONLIBS_FOUND)

# LAMMPS
find_path                   (LAMMPS_INCLUDE_DIR		lammps.h)
find_library                (LAMMPS_LIBRARY NAMES	lammps)
if                          (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  find_path                 (FFTW_INCLUDE_DIR		fftw.h)
  find_library              (FFTW_LIBRARY NAMES	        fftw)
  if                        (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    include_directories     (${LAMMPS_INCLUDE_DIR}
                             ${FFTW_INCLUDE_DIR})
    set                     (libraries
                             ${libraries}
                             ${LAMMPS_LIBRARY}
                             ${FFTW_LIBRARY})
    set                     (lammps_example             ON)
    message                 (STATUS "Found LAMMPS; building LAMMPS example")
  else                      (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
    set                     (lammps_example             OFF)
    message                 (STATUS "Need FFTW (not found) for LAMMPS; not building LAMMPS example")
  endif                     (FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
else                        (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)
  set                       (lammps_example             OFF)
  message                   (STATUS "LAMMPS not found; not building LAMMPS example")
endif                       (LAMMPS_INCLUDE_DIR AND LAMMPS_LIBRARY)

# Set includes
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

add_subdirectory            (examples)
