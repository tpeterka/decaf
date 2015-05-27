find_package(FFS REQUIRED)

include_directories(${FFS_INCLUDE_DIR})

add_executable              (testffs testFFS.cpp)
target_link_libraries       (testffs    ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY})

add_executable              (redist redist.cpp)
target_link_libraries       (redist     ${libraries} ${FFS_LIBRARY} ${FM_LIBRARY})

find_package(Boost COMPONENTS serialization)
if(Boost_FOUND)
    message("Building boost based redistribution example")
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable              (redistboost redist_boost.cpp)
    target_link_libraries       (redistboost     ${libraries} ${Boost_LIBRARIES})

    add_executable              (testboost testBoost.cpp)
    target_link_libraries       (testboost     ${libraries} ${Boost_LIBRARIES})
else(Boost_FOUND)
    message("Boost or Boost serialization component not found. Skipping boost based redistribution example")
endif(Boost_FOUND)


