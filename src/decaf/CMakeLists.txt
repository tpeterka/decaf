add_subdirectory        (C)

add_library             (decaf SHARED       dataflow.cpp
                                            decaf.cpp
                                            tools.cpp
                                            workflow.cpp)

target_link_libraries   (decaf
                        ${libraries} ${EXTRA_FLAGS} bredala_datamodel bredala_transport ${Boost_LIBRARIES} manala)

install                 (TARGETS decaf
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)
