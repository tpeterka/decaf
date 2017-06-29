find_package                (CCI REQUIRED)

include_directories(${CCI_INCLUDE_DIR})

add_library             (bredala_transport_cci SHARED	../redist_comp.cpp
                                                        ../split.cpp
                                                        redist_count_cci.cpp
                                                    	redist_cci.cpp
                                                    	)

target_link_libraries   (bredala_transport_cci
                        ${libraries} ${EXTRA_FLAGS} bredala_datamodel ${Boost_LIBRARIES} ${CCI_LIBRARY})

install                 (TARGETS bredala_transport_cci
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)
