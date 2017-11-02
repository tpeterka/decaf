project                     (Decaf)
cmake_minimum_required      (VERSION 3.0)

option                      (transport_mpi      "Build Decaf with MPI transport layer"          ON)
option                      (transport_cci      "Build Decaf with CCI transport layer"          OFF)
option                      (transport_file     "Build Decaf with file transport layer"         OFF)
option                      (tess_dense         "Build tessellation density estimator example"  OFF)
option                      (build_bredala      "Build Bredala libraries and examples"          ON)
option                      (build_manala       "Build Manala libraries and examples"           ON)
option                      (build_decaf        "Build the Decaf workflow system"               ON)
option                      (build_tests        "Build the tests examples"                      ON)
set                         (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

if(build_manala)
    set                     (build_bredala  true)
endif(build_manala)
if(build_decaf)
    set                     (build_bredala  true)
    set                     (build_manala   true)
endif(build_decaf)

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
    set                     (transport_libraries    ${transport_libraries} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif                     ()
  add_definitions           (-DTRANSPORT_MPI)
  set                       (TRANSPORT_MPI ON)
endif                       (transport_mpi)

#CCI
if                          (transport_cci)
  find_package              (MPI REQUIRED)
  find_package              (CCI REQUIRED)
  set                       (transport_libraries    ${transport_libraries} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES} ${CCI_LIBRARY})
  include_directories       (${CCI_INCLUDE_DIR})
  add_definitions           (-DTRANSPORT_CCI)
  set                       (TRANSPORT_CCI ON)
endif                       (transport_cci)

#FILE
#Should be used with the variable HDF5_PREFER_PARALLEL set to true
if                          (transport_file)
  find_package              (HDF5 REQUIRED)
  include_directories       (${HDF5_INCLUDE_DIR})
  set                       (transport_libraries    ${transport_libraries} ${HDF5_LIBRARIES})
  add_definitions           (-DTRANSPORT_FILE)
  set                       (TRANSPORT_FILE ON)
endif                       (transport_file)

# Boost
find_package                (Boost 1.59.0 COMPONENTS serialization REQUIRED)
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
add_subdirectory            (python)

if                          (build_tests)
  add_subdirectory          (tests)
endif                       ()

# Install the headers
if(build_bredala)
file                        (GLOB DEPLOY_FILES_AND_DIRS "${PROJECT_SOURCE_DIR}/include/bredala/*")
foreach                     (ITEM ${DEPLOY_FILES_AND_DIRS})
   if                       (IS_DIRECTORY "${ITEM}")
      list                  (APPEND DIRS_TO_DEPLOY "${ITEM}")
   else                     ()
      list                  (APPEND FILES_TO_DEPLOY "${ITEM}")
   endif                    ()
endforeach                  ()
install                     (FILES ${FILES_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/bredala)
install                     (DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/bredala)
endif(build_bredala)
if(build_manala)
file                        (GLOB DEPLOY_FILES_AND_DIRS "${PROJECT_SOURCE_DIR}/include/manala/*")
foreach                     (ITEM ${DEPLOY_FILES_AND_DIRS})
   if                       (IS_DIRECTORY "${ITEM}")
      list                  (APPEND DIRS_TO_DEPLOY "${ITEM}")
   else                     ()
      list                  (APPEND FILES_TO_DEPLOY "${ITEM}")
   endif                    ()
endforeach                  ()
install                     (FILES ${FILES_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/manala)
install                     (DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/manala)
endif(build_manala)
if(build_decaf)
file                        (GLOB DEPLOY_FILES_AND_DIRS "${PROJECT_SOURCE_DIR}/include/decaf/*")
foreach                     (ITEM ${DEPLOY_FILES_AND_DIRS})
   if                       (IS_DIRECTORY "${ITEM}")
      list                  (APPEND DIRS_TO_DEPLOY "${ITEM}")
   else                     ()
      list                  (APPEND FILES_TO_DEPLOY "${ITEM}")
   endif                    ()
endforeach                  ()
install                     (FILES ${FILES_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/decaf)
install                     (DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/decaf)
endif(build_decaf)
install                     ( FILES cmake/FindDecaf.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake)

#Configure the config file
configure_file              (include/config_decaf.h.cmakein include/config_decaf.h)
install                     (FILES ${CMAKE_BINARY_DIR}/include/config_decaf.h
                             DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
