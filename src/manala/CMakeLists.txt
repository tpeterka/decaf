add_library             (manala SHARED      datastreaminterface.cpp
                                            datastreamdoublefeedback.cpp
                                            datastreamsinglefeedback.cpp
                                            datastreamsinglenolink.cpp
                                            framemanager.cpp
                                            framemanagerlowhigh.cpp
                                            framemanagermostrecent.cpp
                                            framemanagersequential.cpp
                                            storagecollectionLRU.cpp
                                            storagecollectiongreedy.cpp
                                            storagecollectioninterface.cpp
                                            storagefile.cpp
                                            storagemainmemory.cpp
                                            tools.cpp
                                            )

target_link_libraries   (manala
                        ${libraries} ${EXTRA_FLAGS} bredala_datamodel bredala_transport ${Boost_LIBRARIES})

install                 (TARGETS manala
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)
