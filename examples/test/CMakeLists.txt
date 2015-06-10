find_package(FFS)

if(FFS_FOUND)
    message("Building FFS based redistribution example")
    include_directories(${FFS_INCLUDE_DIR})

    add_executable              (ffs testFFS.cpp)
    target_link_libraries       (ffs    ${libraries} ${FFS_LIBRARY})# ${FM_LIBRARY})

    install(TARGETS ffs
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/test//
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )

else(FFS_FOUND)
    message("FFS not found. Skipping FFS based redistribution example")
endif(FFS_FOUND)
