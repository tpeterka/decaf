project                     (Decaf)
cmake_minimum_required      (VERSION 2.8)

option                      (transport_mpi      "Build Decaf with MPI transport layer"          ON)
option                      (tess_dense         "Build tessellation density estimator example"  OFF)
option                      (bredala_only       "Build only Bredala libraries and examples"     OFF)
option                      (build_tests        "Build the tests examples"                      OFF)
set                         (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# OSX flags
if                          (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions         (-DMAC_OSX)
    set                     (CMAKE_MACOSX_RPATH	  on)

    # --- following RPATH settings are for Sierra w/ Clang, hopefully they don't hurt other versions
    # ref: https://cmake.org/Wiki/CMake_RPATH_handling
    # use, i.e. don't skip, the full RPATH for the build tree
    set                     (CMAKE_SKIP_BUILD_RPATH            false)
    # when building, don't use the install RPATH already (but later on when installing)
    set                     (CMAKE_BUILD_WITH_INSTALL_RPATH    false)
    # set RPATH to install path
    set                     (CMAKE_INSTALL_RPATH               "${CMAKE_INSTALL_PREFIX}/lib")
    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    set                     (CMAKE_INSTALL_RPATH_USE_LINK_PATH true)
    # the RPATH to be used when installing, but only if it's not a system directory
    list                    (FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
                                                               "${CMAKE_INSTALL_PREFIX}/lib"
                                                               isSystemDir)
    if                      ("${isSystemDir}" STREQUAL         "-1")
      set                   (CMAKE_INSTALL_RPATH               "${CMAKE_INSTALL_PREFIX}/lib")
    endif                   ()
endif                       (${CMAKE_SYSTEM_NAME} MATCHES      "Darwin")

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
message                     (STATUS "Boost libraries: " ${Boost_LIBRARIES})

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

set (CMAKE_LINKER_FLAGS ${CMAKE_LINKER_FLAGS} "-Wl,--export-dynamic -dynamic")

# subdirectories
add_subdirectory            (src)
add_subdirectory            (examples)
message                     (STATUS "Examples configured")
if                          (PYTHONLIBS_FOUND)
  add_subdirectory          (python)
endif                       ()

if                          (build_tests)
  add_subdirectory          (tests)
endif                       ()

# Install the headers
file                        (GLOB DEPLOY_FILES_AND_DIRS "${PROJECT_SOURCE_DIR}/include/*")
foreach                     (ITEM ${DEPLOY_FILES_AND_DIRS})
   if                       (IS_DIRECTORY "${ITEM}")
      list                  (APPEND DIRS_TO_DEPLOY "${ITEM}")
   else                     ()
      list                  (APPEND FILES_TO_DEPLOY "${ITEM}")
   endif                    ()
endforeach                  ()
install                     (FILES ${FILES_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install                     (DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install                     ( FILES cmake/FindDecaf.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake)
