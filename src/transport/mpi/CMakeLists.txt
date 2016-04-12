find_package            (Boost COMPONENTS serialization REQUIRED)
include_directories     (${Boost_INCLUDE_DIRS})

add_library             (decaf_transport SHARED     ../redist_comp.cpp
                                                    redist_count_mpi.cpp
                                                    redist_round_mpi.cpp
                                                    redist_zcurve_mpi.cpp
						    redist_block_mpi.cpp)

target_link_libraries   (decaf_transport            ${libraries} ${Boost_LIBRARIES} decaf_datamodel)

install(TARGETS decaf_transport
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)


