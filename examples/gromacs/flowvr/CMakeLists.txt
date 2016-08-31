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

    FIND_PACKAGE(Qt4 REQUIRED)
    FIND_PACKAGE(NetCDF REQUIRED)
    FIND_PACKAGE(VRPN REQUIRED)
    FIND_PACKAGE(OpenGL REQUIRED)
    FIND_PACKAGE(GLUT REQUIRED)
    FIND_PACKAGE(GLEW REQUIRED)

    INCLUDE_DIRECTORIES(./include ${NetCDF_INCLUDE_DIRECTORIES} ${QT_INCLUDES} ${FLOWVR_INCLUDE_DIR})

    ADD_EXECUTABLE(NetCDFMolRepReaderDecaf NetCDFMolRepReader.cpp main.cpp)
    TARGET_LINK_LIBRARIES(NetCDFMolRepReaderDecaf ${NetCDF_C_LIBRARY} ${NetCDF_CXX_LIBRARY} ${FLOWVR_base_LIBRARY} ${FLOWVR_ftl_LIBRARY} ${FLOWVR_mod_LIBRARY} ${QT_QTCORE_LIBRARY})
    INSTALL(TARGETS NetCDFMolRepReaderDecaf DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/)

    INCLUDE_DIRECTORIES(${FLOWVR_INCLUDE_DIR} ${QT_INCLUDES} ${FLOWVR_VRPN_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${VRPN_INCLUDES} ${QUAT_INCLUDES})

    ADD_EXECUTABLE(RenduOpenGLDecaf RenduSimple.cpp Vector3D.cpp Deselector.cpp AtomAndBondShaders.cpp AtomAndBondGLSL.cpp)

    TARGET_LINK_LIBRARIES(RenduOpenGLDecaf ${QT_QTCORE_LIBRARY} ${FLOWVR_base_LIBRARY} ${FLOWVR_mod_LIBRARY} ${FLOWVRVRPN_LIBRARY} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES}  ${GLEW_LIBRARIES} )

    INSTALL(TARGETS RenduOpenGLDecaf  DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gromacs/)

else(FLOWVR_FOUND)
    message(STATUS "FlowVR not found")
endif(FLOWVR_FOUND)

