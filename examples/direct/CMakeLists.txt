add_executable              (direct            direct.cpp)
add_library                 (mod_direct MODULE direct.cpp)
target_link_libraries       (direct     ${libraries}     )

add_subdirectory            (python)
