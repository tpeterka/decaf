# include_directories     (${Boost_INCLUDE_DIRS})

# DEPRECATED, don't force shared; let the user decide
# add_library             (bredala_transport SHARED     ../redist_comp.cpp
#                                                     redist_mpi.cpp
#                                                     redist_count_mpi.cpp
#                                                     redist_round_mpi.cpp
#                                                     redist_zcurve_mpi.cpp
#                                                     redist_block_mpi.cpp
#                                                     redist_proc_mpi.cpp)

add_library             (bredala_transport          ../redist_comp.cpp
                                                    redist_mpi.cpp
                                                    redist_count_mpi.cpp
                                                    redist_round_mpi.cpp
                                                    redist_zcurve_mpi.cpp
                                                    redist_block_mpi.cpp
                                                    redist_proc_mpi.cpp)

target_link_libraries   (bredala_transport 
  ${libraries} ${EXTRA_FLAGS} bredala_datamodel ${Boost_LIBRARIES})

install                 (TARGETS bredala_transport
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)


