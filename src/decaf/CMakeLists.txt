add_subdirectory        (C)

add_library             (decaf SHARED       dataflow.cpp
                                            decaf.cpp
                                            tools.cpp
                                            workflow.cpp)

set(bredala_transports "")
if(${transport_mpi})
    list(APPEND bredala_transports bredala_transport_mpi)
endif(${transport_mpi})
if(${transport_cci})
    list(APPEND bredala_transports bredala_transport_cci)
endif(${transport_cci})
if(${transport_file})
    list(APPEND bredala_transports bredala_transport_file)
endif(${transport_file})

target_link_libraries   (decaf
                        ${libraries} ${EXTRA_FLAGS} bredala_datamodel ${bredala_transports} ${Boost_LIBRARIES} manala)

install                 (TARGETS decaf
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)
