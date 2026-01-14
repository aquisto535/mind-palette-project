cmake_minimum_required(VERSION 3.15)
project(PreprocessServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Dependencies (Managed by vcpkg via vcpkg.json)
# Visual Studio will automatically install these packages when opening the folder.
find_package(Crow CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Main Executable
add_executable(preprocess_server src/main.cpp)
target_link_libraries(preprocess_server PRIVATE Crow::Crow asio::asio Threads::Threads)
target_include_directories(preprocess_server PRIVATE src)

# Testing
enable_testing()

find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests tests/test_main.cpp)
target_link_libraries(unit_tests PRIVATE Crow::Crow asio::asio GTest::gtest_main Threads::Threads)
target_include_directories(unit_tests PRIVATE src)

include(GoogleTest)
gtest_discover_tests(unit_tests)
