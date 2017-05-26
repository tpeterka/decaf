add_library             (bredala_datamodel SHARED     constructtype.cpp
                        morton.cpp maptools.cpp)

target_link_libraries   (bredala_datamodel
                        ${libraries} ${EXTRA_FLAGS} ${Boost_LIBRARIES})

install                 (TARGETS bredala_datamodel
                        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
                        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_WRITE GROUP_EXECUTE
                        WORLD_READ WORLD_WRITE WORLD_EXECUTE)


