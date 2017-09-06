add_library             (bredala_transport_file SHARED	../redist_comp.cpp
                                                        ../split.cpp
                                                        redist_count_file.cpp
                                                        redist_round_file.cpp
                                                        redist_proc_file.cpp
                                                        redist_block_file.cpp
                                                    	redist_file.cpp
                                                    	)

target_link_libraries   (bredala_transport_file
                        ${libraries} ${EXTRA_FLAGS} bredala_datamodel ${Boost_LIBRARIES})

install                 (TARGETS bredala_transport_file
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)
