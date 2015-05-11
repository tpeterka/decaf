add_executable              (direct            direct.cpp)
add_library                 (mod_direct MODULE direct.cpp)
target_link_libraries       (direct     ${libraries}     )

if                          (python_examples)
  add_subdirectory          (python)
endif                       (python_examples)
