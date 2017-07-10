add_library             (bredala_transport_mpi SHARED     ../redist_comp.cpp
                                                    ../split.cpp
                                                    redist_mpi.cpp
                                                    redist_count_mpi.cpp
                                                    redist_round_mpi.cpp
                                                    redist_zcurve_mpi.cpp
                                                    redist_block_mpi.cpp
                                                    redist_proc_mpi.cpp
                                                    comm.cpp
                                                    channel.cpp)

target_link_libraries   (bredala_transport_mpi
                        ${libraries} ${EXTRA_FLAGS} bredala_datamodel ${Boost_LIBRARIES})

install                 (TARGETS bredala_transport_mpi
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)


