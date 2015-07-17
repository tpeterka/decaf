#add_executable              (cfd            cfd.cpp)
#add_library                 (mod_cfd MODULE cfd.cpp)

#target_link_libraries       (cfd      ${libraries} decaf_transport )
#target_link_libraries       (mod_cfd  ${libraries} decaf_transport )

Message("Decaf transport : ${decaf_transport}")

if                          (python_examples)
    add_subdirectory          (python)
endif                       (python_examples)

#install(TARGETS cfd mod_cfd
#        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/
#        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

#install(FILES CFD_TEST
#        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/
#        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building cfd example with Redistribution component")
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable              (cfd_redist     cfd_redist.cpp cfd_kernel.cpp)
    add_library                 (mod_cfd_redist MODULE cfd_redist.cpp cfd_kernel.cpp)
    target_link_libraries       (cfd_redist     ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)
    target_link_libraries       (mod_cfd_redist ${libraries} ${Boost_LIBRARIES} decaf_transport decaf_datamodel)

    install(TARGETS cfd_redist mod_cfd_redist
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

        install(FILES CFD_REDIST_TEST
            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/cfd/
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

endif(Boost_FOUND)
