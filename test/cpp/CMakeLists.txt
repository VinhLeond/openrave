if (NOT ${OPT_TEST} STREQUAL "ON")
    return()
endif()

cmake_minimum_required(VERSION 3.20)

enable_testing()
find_package(Boost 1.74 REQUIRED COMPONENTS program_options)
find_package(GTest 1.10 REQUIRED)

set(OPENRAVE_PLUGIN_PATH "/opt/lib/openrave${OPENRAVE_VERSION_MAJOR}.${OPENRAVE_VERSION_MINOR}-plugins")
message(STATUS "Default OpenRAVE plugin path for unit tests is ${OPENRAVE_PLUGIN_PATH}")

set(test_sources
    main.cpp
    test_benchmarks.cpp
    test_interface.cpp
    test_collision.cpp
)

add_executable(openrave_tests ${test_sources})
target_include_directories(openrave_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/libopenrave-core
)
target_link_libraries(openrave_tests PRIVATE
    libopenrave
    libopenrave-core 
    GTest::gtest

    Boost::program_options
)
target_compile_definitions(openrave_tests PRIVATE "OPENRAVE_DEFAULT_PLUGIN_PATH=\"${OPENRAVE_PLUGIN_PATH}\"")

add_test(openrave_tests openrave_tests)

install(TARGETS openrave_tests EXPORT openrave-targets RUNTIME DESTINATION bin)
