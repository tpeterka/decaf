project                     (Decaf)
cmake_minimum_required      (VERSION 2.8)

option                      (debug              "Build Decaf with debugging on"                OFF)
option                      (optimize           "Build Decaf with optimization"                OFF)
option                      (transport_mpi      "Build Decaf with MPI transport layer"          ON)

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

# Set includes
set                         (CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem")
include_directories         (${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}/include
                             SYSTEM ${MPI_INCLUDE_PATH})

# Set libraries
set                         (libraries
                             ${transport_libraries})

add_subdirectory            (examples)
