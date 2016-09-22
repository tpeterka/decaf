# if                          (python_examples)
#   add_subdirectory          (python)
# endif                       (python_examples)

find_package                  (Boost COMPONENTS serialization REQUIRED)
if                            (${serial} MATCHES "CGAL")
  find_package                (CGAL)
  find_package                (Boost)
endif                         ()

if(Boost_FOUND)

  message("Building tess_dense (mpmd) example")
  include_directories(${Boost_INCLUDE_DIRS} ${TESS_INCLUDE_DIR} ${DIY_INCLUDE_DIR})

  add_executable              (points             prod.cpp)
  add_executable              (tess               tess.cpp)
  add_executable              (dense              dense.cpp)
  add_executable              (pts_dflow          dflow.cpp)
  add_library                 (mod_pts_dflow      MODULE dflow.cpp)

  target_link_libraries       (points
                              ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  if                          (${serial} MATCHES "CGAL")
    include                   (${CGAL_USE_FILE})
    add_definitions           (${CGAL_CXX_FLAGS_INIT})
    include_directories       (${CGAL_INCLUDE_DIRS} SYSTEM ${Boost_INCLUDE_DIR})
    target_link_libraries     (tess
                               ${libraries}
                               ${Boost_LIBRARIES}
                               ${TESS_LIBRARY}
                               ${CGAL_LIBRARY}
                               ${CGAL_3RD_PARTY_LIBRARIES}
                               bredala_transport
                               bredala_datamodel)
    target_link_libraries     (dense
                               ${libraries}
                               ${Boost_LIBRARIES}
                               ${TESS_LIBRARY}
                               ${CGAL_LIBRARY}
                               ${CGAL_3RD_PARTY_LIBRARIES}
                               bredala_transport
                               bredala_datamodel)
  add_definitions             (-DTESS_CGAL_ALLOW_SPATIAL_SORT)
  add_definitions             (-DTESS_USE_CGAL)
  else                        ()
    target_link_libraries     (tess
                               ${libraries}
                               ${Boost_LIBRARIES}
                               ${TESS_LIBRARY}
                               ${QHull_LIBRARY}
                               bredala_transport
                               bredala_datamodel)
    target_link_libraries     (dense
                               ${libraries}
                               ${Boost_LIBRARIES}
                               ${TESS_LIBRARY}
                               ${QHull_LIBRARY}
                               bredala_transport
                               bredala_datamodel)
  endif                       ()
  target_link_libraries       (pts_dflow
                               ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
  target_link_libraries       (mod_pts_dflow
                               ${libraries} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)

  # .SO file extension on Linux/Mac OS
  set_target_properties       (mod_pts_dflow PROPERTIES SUFFIX ".so")

  # Don't add a 'lib' prefix to the shared library
  set_target_properties       (mod_pts_dflow PROPERTIES PREFIX "")

  install                     (TARGETS
                               points
                               tess
                               dense
                               pts_dflow
                               mod_pts_dflow
                               DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/tess_dense/
                               PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                               GROUP_READ GROUP_WRITE GROUP_EXECUTE
                               WORLD_READ WORLD_WRITE WORLD_EXECUTE)

  install                     (FILES TESS_DENSE_TEST
                               DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/tess_dense/
                               PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                               GROUP_READ GROUP_WRITE GROUP_EXECUTE
                               WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
