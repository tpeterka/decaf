add_executable              (direct direct.cpp)
add_library                 (modules MODULE direct.cpp)
target_link_libraries       (direct     ${libraries})

add_subdirectory            (python)
