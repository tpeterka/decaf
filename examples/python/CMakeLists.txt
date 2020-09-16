message("Building python example")


install                     (FILES
                            linear-2nodes.py prod.py con.py
                            DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/python/
                            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_WRITE GROUP_EXECUTE
                            WORLD_READ WORLD_WRITE WORLD_EXECUTE)
