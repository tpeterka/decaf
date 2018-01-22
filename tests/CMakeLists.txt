#find_package(FFS)

#if(FFS_FOUND)
#    message("Building FFS based redistribution example")
#    include_directories(${FFS_INCLUDE_DIR})

#    add_executable              (testffs testFFS.cpp)
#    target_link_libraries       (testffs    ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY})

#    add_executable              (redistffs redist.cpp)
#    target_link_libraries       (redistffs     ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY} ${bredala_transports} bredala_datamodel)
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

if(${transport_mpi})
	add_executable              (testboost testBoost.cpp)
	target_link_libraries       (testboost ${libraries} ${bredala_transports} bredala_datamodel)

	#    add_executable              (redistboost redist_boost.cpp)
	#    target_link_libraries       (redistboost     ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testzcurve testZCurve.cpp)
	target_link_libraries       (testzcurve ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testround testRound.cpp)
	target_link_libraries       (testround ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testcount testCount.cpp)
	target_link_libraries       (testcount ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testemptymessage testEmptyMessage.cpp)
	target_link_libraries       (testemptymessage ${libraries} ${bredala_transports} bredala_datamodel)


	#    add_executable              (testarrayndim testArrayNDim.cpp)
	#    target_link_libraries       (testarrayndim     ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testblock testBlock.cpp)
	target_link_libraries       (testblock ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testproc testProc.cpp)
	target_link_libraries       (testproc ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testmem testmem.cpp)
	target_link_libraries       (testmem ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testserialization testSerialization.cpp)
	target_link_libraries       (testserialization ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testcountoverflow testCountOverflow.cpp)
	target_link_libraries       (testcountoverflow ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testroundoverflow testRoundOverflow.cpp)
	target_link_libraries       (testroundoverflow ${libraries} ${bredala_transports} bredala_datamodel)

	install(TARGETS testboost testzcurve testround testcount testblock testproc testmem testserialization testemptymessage testcountoverflow testroundoverflow
  	DESTINATION ${CMAKE_INSTALL_PREFIX}/tests/
  	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
  	)
	install(FILES BOOST_TEST ZCURVE_TEST ROUND_TEST COUNT_TEST BLOCK_TEST PROC_TEST EMPTY_TEST OVERFLOW_COUNT_TEST OVERFLOW_ROUND_TEST gdb.run
  	DESTINATION ${CMAKE_INSTALL_PREFIX}/tests/
  	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
  	)
endif(${transport_mpi})

if(${transport_cci})

    add_executable              (testcountcci testCountCCI.cpp)
	target_link_libraries	    (testcountcci ${libraries} ${bredala_transports} bredala_datamodel)

	add_executable              (testroundcci testRoundCCI.cpp)
	target_link_libraries       (testroundcci ${libraries} ${bredala_transports} bredala_datamodel)

    add_executable              (testproccci testProcCCI.cpp)
	target_link_libraries	    (testproccci ${libraries} ${bredala_transports} bredala_datamodel)

    add_executable              (testblockcci testBlockCCI.cpp)
	target_link_libraries	    (testblockcci ${libraries} ${bredala_transports} bredala_datamodel)

	install(TARGETS testroundcci testcountcci testproccci testblockcci
  	DESTINATION ${CMAKE_INSTALL_PREFIX}/tests/
  	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
  	)

endif(${transport_cci})

if(${transport_file})
    add_executable              (testprocfile testProcFile.cpp)
    target_link_libraries	    (testprocfile ${libraries} ${bredala_transports} bredala_datamodel)

    add_executable              (testcountfile testCountFile.cpp)
    target_link_libraries	    (testcountfile ${libraries} ${bredala_transports} bredala_datamodel)

    add_executable              (testroundfile testRoundFile.cpp)
    target_link_libraries	    (testroundfile ${libraries} ${bredala_transports} bredala_datamodel)

    add_executable              (testblockfile testBlockFile.cpp)
    target_link_libraries	    (testblockfile ${libraries} ${bredala_transports} bredala_datamodel)

    install(TARGETS testprocfile testcountfile testroundfile testblockfile
    DESTINATION ${CMAKE_INSTALL_PREFIX}/tests/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE
    )
endif(${transport_file})


