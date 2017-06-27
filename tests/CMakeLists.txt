#find_package(FFS)

#if(FFS_FOUND)
#    message("Building FFS based redistribution example")
#    include_directories(${FFS_INCLUDE_DIR})

#    add_executable              (testffs testFFS.cpp)
#    target_link_libraries       (testffs    ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY})

#    add_executable              (redistffs redist.cpp)
#    target_link_libraries       (redistffs     ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY} bredala_transport bredala_datamodel)
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

message("Building boost based redistribution example")

find_package(CCI REQUIRED)
include_directories(${CCI_INCLUDE_DIR})

add_executable              (testboost testBoost.cpp)
target_link_libraries       (testboost ${libraries} bredala_transport bredala_datamodel)

#    add_executable              (redistboost redist_boost.cpp)
#    target_link_libraries       (redistboost     ${libraries} bredala_transport bredala_datamodel)

add_executable              (testzcurve testZCurve.cpp)
target_link_libraries       (testzcurve ${libraries} bredala_transport bredala_datamodel)

add_executable              (testround testRound.cpp)
target_link_libraries       (testround ${libraries} bredala_transport bredala_datamodel)

add_executable              (testcount testCount.cpp)
target_link_libraries       (testcount ${libraries} bredala_transport bredala_datamodel)

add_executable              (testemptymessage testEmptyMessage.cpp)
target_link_libraries       (testemptymessage ${libraries} bredala_transport bredala_datamodel)


#    add_executable              (testarrayndim testArrayNDim.cpp)
#    target_link_libraries       (testarrayndim     ${libraries} bredala_transport bredala_datamodel)

add_executable              (testblock testBlock.cpp)
target_link_libraries       (testblock ${libraries} bredala_transport bredala_datamodel)

add_executable              (testproc testProc.cpp)
target_link_libraries       (testproc ${libraries} bredala_transport bredala_datamodel)

add_executable              (testmem testmem.cpp)
target_link_libraries       (testmem ${libraries} bredala_transport bredala_datamodel)

add_executable              (testserialization testSerialization.cpp)
target_link_libraries       (testserialization ${libraries} bredala_transport bredala_datamodel)

add_executable		    (testcci testCCI.cpp)
target_link_libraries	    (testcci ${libraries} bredala_transport bredala_transport_cci bredala_datamodel)

install(TARGETS testboost testzcurve testround testcount testblock testproc testmem testserialization testemptymessage testcci
  DESTINATION ${CMAKE_INSTALL_PREFIX}/tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
  )
install(FILES BOOST_TEST ZCURVE_TEST ROUND_TEST COUNT_TEST BLOCK_TEST PROC_TEST EMPTY_TEST gdb.run
  DESTINATION ${CMAKE_INSTALL_PREFIX}/tests/
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
  )


