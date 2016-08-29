find_package(FlowVR)
if(FLOWVR_FOUND)
    include_directories(${FLOWVR_INCLUDE_DIR})
    message(STATUS "FlowVR found: ${FLOWVR_INCLUDE_DIR}")
    list(APPEND DEP_LIBRARIES ${FLOWVR_base_LIBRARY} ${FLOWVR_mod_LIBRARY})

    add_executable              (treatment_flowvr          treatment_flowvr.cpp)
    add_executable              (targetmanager_flowvr      targetmanager_flowvr.cpp)
    target_link_libraries       (treatment_flowvr
            ${libraries} decaf_transport decaf_datamodel ${DEP_LIBRARIES})
    target_link_libraries       (targetmanager_flowvr
            ${libraries} decaf_transport decaf_datamodel ${DEP_LIBRARIES})

    install(TARGETS treatment_flowvr targetmanager_flowvr
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_WRITE GROUP_EXECUTE
        WORLD_READ WORLD_WRITE WORLD_EXECUTE)

    install(FILES fepa_flowvr.py
        DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_WRITE GROUP_EXECUTE
        WORLD_READ WORLD_WRITE WORLD_EXECUTE)

else(FLOWVR_FOUND)
    message(STATUS "FlowVR not found")
endif(FLOWVR_FOUND)

