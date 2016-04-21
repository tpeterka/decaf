#find_package(FFS)

#if(FFS_FOUND)
#    message("Building FFS based redistribution example")
#    include_directories(${FFS_INCLUDE_DIR})

#    add_executable              (testffs testFFS.cpp)
#    target_link_libraries       (testffs    ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY})

#    add_executable              (redistffs redist.cpp)
#    target_link_libraries       (redistffs     ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY} decaf_transport decaf_datamodel)
#    install(TARGETS testffs redistffs
#            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/redist/
#            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
#            )
#    install(FILES FFS_TEST REDISTFFS_TEST
#            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/redist/
#            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
#            )

#else(FFS_FOUND)
#    message("FFS not found. Skipping FFS based redistribution example")
#endif(FFS_FOUND)

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building boost based redistribution example")
    include_directories(${Boost_INCLUDE_DIRS})

    add_executable              (testboost testBoost.cpp)
    target_link_libraries       (testboost     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

#    add_executable              (redistboost redist_boost.cpp)
#    target_link_libraries       (redistboost     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    add_executable              (testzcurve testZCurve.cpp)
    target_link_libraries       (testzcurve     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    add_executable              (testround testRound.cpp)
    target_link_libraries       (testround     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    add_executable              (test2count test2Count.cpp)
    target_link_libraries       (test2count     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

#    add_executable              (testarrayndim testArrayNDim.cpp)
#    target_link_libraries       (testarrayndim     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    add_executable              (testblock testBlock.cpp)
    target_link_libraries       (testblock     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    install(TARGETS testboost testzcurve testround test2count testblock
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/redist/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )
    install(FILES BOOST_TEST ZCURVE_TEST ROUND_TEST COUNT_TEST BLOCK_TEST gdb.run
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/redist/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
            )

else(Boost_FOUND)
    message("Boost or Boost serialization component not found. Skipping boost based redistribution example")
endif(Boost_FOUND)


