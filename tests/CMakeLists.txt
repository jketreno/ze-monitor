cmake_minimum_required(VERSION 3.10)
project(ze_monitor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../src
)

# Find Catch2 package if available
find_package(Catch2 QUIET)

# Test executable
add_executable(tests
    test_main.cpp
    test_temperature.cpp
    ze_mock.cpp
    ../src/temperature.cpp  # Include the implementation directly
    ../src/helpers.cpp
)

target_include_directories(tests PRIVATE ../)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

# Enable testing with CTest
enable_testing()
add_test(NAME AllTests COMMAND tests)
