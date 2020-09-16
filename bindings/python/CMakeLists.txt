pybind11_add_module         (pydecaf py-decaf.cpp)
target_link_libraries       (pydecaf PRIVATE
                             ${libraries} ${bredala_transports} bredala_datamodel manala decaf ${Boost_LIBRARIES})

pybind11_add_module         (pybredala py-bredala.cpp)
target_compile_options      (pybredala PUBLIC "-fvisibility=default")
target_link_libraries       (pybredala PRIVATE
                             ${libraries} bredala_datamodel)

install                     (TARGETS pydecaf pybredala
                            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/python/
                            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_WRITE GROUP_EXECUTE
                            WORLD_READ WORLD_WRITE WORLD_EXECUTE)
