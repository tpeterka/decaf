
find_package(Boost COMPONENTS serialization REQUIRED)

# DIY
find_path                   (DIY_INCLUDE_DIRS       diy/types.h HINTS ${DIY_PREFIX})

if(Boost_FOUND)
    message("Building boost based redistribution example")
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${DIY_INCLUDE_DIRS} ${TESS_INCLUDE_DIRS})

    add_executable              (block_data_model block_data_model.cpp)
    target_link_libraries       (block_data_model     ${libraries} ${Boost_LIBRARIES} ${TESS_LIBRARY} decaf_transport decaf_datamodel)

    install(TARGETS block_data_model 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/block_data_model/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )
    install(FILES BLOCKDATAMODEL 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/block_data_model/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )

else(Boost_FOUND)
    message("Boost or Boost serialization component not found. Skipping boost based redistribution example")
endif(Boost_FOUND)


