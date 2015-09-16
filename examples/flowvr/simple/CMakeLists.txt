# INSTALLATION DIRECTORIES

SET(INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
SET(COMP_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/tictac/components)  # Use namespace to define directory: include/name/space/components
SET(LIB_DIR ${CMAKE_INSTALL_PREFIX}/examples/flowvr/simple/)
SET(COMP_LIB_DIR ${CMAKE_INSTALL_PREFIX}/examples/flowvr/simple/)
SET(BIN_DIR ${CMAKE_INSTALL_PREFIX}/examples/flowvr/simple/)
SET(MODULE_DIR ${CMAKE_INSTALL_PREFIX}/examples/flowvr/simple/)
SET(CMAKE_DIR ${CMAKE_INSTALL_PREFIX}/examples/flowvr/simple/cmake) # Use namespace to define directory: /share/name/space/cmake




# CHECK FLOWVR DEPENDENCY

FIND_PACKAGE(FlowVR REQUIRED)
IF(FLOWVR_FOUND)
 MESSAGE(STATUS "FlowVR Found")
ELSE(FLOWVR_FOUND)
 MESSAGE(SEND_ERROR "FlowVR Not Found")
ENDIF(FLOWVR_FOUND)




# Configuration Files 

FOREACH(shell sh)
  CONFIGURE_FILE(config/app-config.${shell}.in ${CMAKE_CURRENT_BINARY_DIR}/tictac-config.${shell} @ONLY)
  INSTALL(PROGRAMS  ${CMAKE_CURRENT_BINARY_DIR}/tictac-config.${shell} DESTINATION ${BIN_DIR})
ENDFOREACH(shell sh)

CONFIGURE_FILE(config/appConfig.cmake.in  ${CMAKE_CURRENT_BINARY_DIR}/tictacConfig.cmake @ONLY)
INSTALL(FILES  ${CMAKE_CURRENT_BINARY_DIR}/tictacConfig.cmake   DESTINATION ${CMAKE_DIR})



ADD_SUBDIRECTORY(src)


INSTALL(FILES tictac.py DESTINATION ${BIN_DIR})

INSTALL(CODE "MESSAGE(STATUS \"!!!Rember to execute \n. ${BIN_DIR}/tictac-config.sh\nto properly run the examples \n(include it in your .bashrc for a permanent installation)\") ")
