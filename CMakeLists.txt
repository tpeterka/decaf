project                     (Decaf)
cmake_minimum_required      (VERSION 2.8)

option                      (debug              "Build Decaf with debugging on"                ON)
option                      (optimize           "Build Decaf with optimization"                OFF)
option                      (transport_mpi      "Build Decaf with MPI transport layer"         ON)
option                      (CYTHON_PREFIX      "Path to Cython directory"                       )

set                         (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# C++11
set                         (CMAKE_CXX_STANDARD        11)

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
  add_definitions           (-DDEBUG -DDECAF_DEBUG_ON)
else                        (debug)
  if                        (optimize)
    set                     (cxx_flags                  ${CMAKE_CXX_FLAGS_RELEASE})
  else                      (optimize)
    set                     (cxx_flags                  ${CMAKE_CXX_FLAGS})
  endif                     (optimize)
endif                       (debug)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
    "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") #Syntax for CMAKE 3.+
  set(cxx_flags "${cxx_flags} -std=c++11 -stdlib=libc++")
  set                     (CMAKE_EXE_LINKER_FLAGS     "-lc++")
  set                     (CMAKE_MODULE_LINKER_FLAGS  "-lc++")
  set                     (CMAKE_SHARED_LINKER_FLAGS  "-lc++")
else()
    set(cxx_flags "${cxx_flags} -std=c++11")
endif()

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

# subdirectories
add_subdirectory            (src)
add_subdirectory            (examples)
if                          (Cython_FOUND AND PYTHONLIBS_FOUND)
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
