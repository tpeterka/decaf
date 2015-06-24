find_package(Boost COMPONENTS serialization REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_library             (decaf_datamodel SHARED     constructtype.cpp)
target_link_libraries   (decaf_datamodel            ${libraries} ${Boost_LIBRARIES})

install(TARGETS decaf_datamodel
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)


