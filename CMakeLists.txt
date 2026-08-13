cmake_minimum_required(VERSION 3.16)

project(cross_mcu_sensor_framework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sensor_demo
    src/main.cpp
)

target_include_directories(sensor_demo PRIVATE include)

option(BUILD_TESTING "Build framework tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_executable(sensor_framework_tests tests/framework_tests.cpp)
    target_include_directories(sensor_framework_tests PRIVATE include)
    add_test(NAME sensor_framework_tests COMMAND sensor_framework_tests)
endif()
