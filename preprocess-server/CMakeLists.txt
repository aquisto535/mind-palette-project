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
find_package(OpenCV REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Main Executable
add_executable(preprocess_server src/main.cpp src/image_processor.cpp src/utils/Logger.cpp)
target_link_libraries(preprocess_server PRIVATE Crow::Crow asio::asio Threads::Threads ${OpenCV_LIBS} spdlog::spdlog)
target_include_directories(preprocess_server PRIVATE src)

# Week 3 Manual Test Executable
add_executable(test_week3 src/test_week3.cpp src/image_processor.cpp)
target_link_libraries(test_week3 PRIVATE ${OpenCV_LIBS})
target_include_directories(test_week3 PRIVATE src)

# Week 3 Pipeline Test
add_executable(test_pipeline src/test_pipeline.cpp src/image_processor.cpp)
target_link_libraries(test_pipeline PRIVATE ${OpenCV_LIBS})
target_include_directories(test_pipeline PRIVATE src)

# Testing
enable_testing()

find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests tests/test_main.cpp src/image_processor.cpp src/utils/Logger.cpp)
target_link_libraries(unit_tests PRIVATE Crow::Crow asio::asio GTest::gtest_main Threads::Threads ${OpenCV_LIBS} spdlog::spdlog)
target_include_directories(unit_tests PRIVATE src)

include(GoogleTest)
gtest_discover_tests(unit_tests)
