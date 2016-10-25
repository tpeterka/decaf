# DIY
find_path                   (DIY_INCLUDE_DIRS       diy/types.h HINTS ${DIY_PREFIX})

if(DIY_INCLUDE_DIRS AND TESS_LIBRARY AND TESS_INCLUDE_DIRS)
    message("Building block data model example")
    include_directories(${DIY_INCLUDE_DIRS} ${TESS_INCLUDE_DIRS})
    add_executable              (block_data_model block_data_model.cpp)
    target_link_libraries       (block_data_model     ${libraries} ${Boost_LIBRARIES} ${TESS_LIBRARY} bredala_transport bredala_datamodel)

    install(TARGETS block_data_model
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/block_data_model/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )
    install(FILES BLOCKDATAMODEL_TEST
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/block_data_model/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )

else(DIY_INCLUDE_DIRS AND  TESS_LIBRARY AND TESS_INCLUDE_DIRS)
    if(NOT DIY_INCLUDE_DIRS)
        message("DIY includes not found. Please set the variable DIY_PREFIX to the include folder of DIY2.")
    endif(NOT DIY_INCLUDE_DIRS)
    if(NOT TESS_LIBRARY)
        message("Tess library not found. Please set the variable TESS_LIBRARY to the path of libtess.so")
    endif(NOT TESS_LIBRARY)
    if(NOT TESS_INCLUDE_DIRS)
        message("Tess header not found. Please set the variable TESS_INCLUDE_DIRS to the include folder of Tess.")
    endif(NOT TESS_INCLUDE_DIRS)
    message("Skipping block data model example.")
endif(DIY_INCLUDE_DIRS AND TESS_LIBRARY AND TESS_INCLUDE_DIRS)


