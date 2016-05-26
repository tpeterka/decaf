ADD_EXECUTABLE(SimpleBCA simplebca.c)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra")

TARGET_LINK_LIBRARIES(SimpleBCA bca)

INSTALL(TARGETS SimpleBCA DESTINATION examples/C/)

