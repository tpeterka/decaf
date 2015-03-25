set(CMAKE_CXX_FLAGS "-g")
add_executable              (datatypes datatypes.cpp)
target_link_libraries       (datatypes     ${libraries})

add_subdirectory            (python)

