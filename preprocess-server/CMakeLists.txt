cmake_minimum_required(VERSION 3.15)

# Static Linking Configuration (resolves OpenCV DLL issues)
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

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
set(FILTER_SOURCES
    src/filters/resize_filter.cpp
    src/filters/denoise_filter.cpp
    src/filters/grayscale_filter.cpp
    src/filters/canny_filter.cpp
    src/filters/binarize_filter.cpp
    src/filters/morphology_filter.cpp
    src/filters/invert_filter.cpp
    src/filters/rgb_convert_filter.cpp
    src/core/filter_pipeline.cpp
    src/core/pipeline_factory.cpp
)

# Week 4 Concurrency Sources
set(WEEK4_SOURCES
    src/infra/thread_pool.cpp
    src/infra/atomic_writer.cpp
    src/infra/benchmark.cpp
)

add_executable(preprocess_server src/main.cpp src/core/image_processor.cpp src/utils/Logger.cpp ${FILTER_SOURCES})
target_link_libraries(preprocess_server PRIVATE Crow::Crow asio::asio Threads::Threads ${OpenCV_LIBS} spdlog::spdlog)
target_include_directories(preprocess_server PRIVATE src src/core src/infra)

# Week 3 Manual Test Executable
add_executable(test_week3 examples/test_week3.cpp src/core/image_processor.cpp)
target_link_libraries(test_week3 PRIVATE ${OpenCV_LIBS})
target_include_directories(test_week3 PRIVATE src src/core)

# Week 3 Pipeline Test (with GrabCut)
add_executable(test_pipeline examples/test_pipeline.cpp src/core/image_processor.cpp)
target_link_libraries(test_pipeline PRIVATE ${OpenCV_LIBS})
target_include_directories(test_pipeline PRIVATE src src/core)

# Week 3 Canny Pipeline Test (GrabCut excluded - production version)
add_executable(test_canny_pipeline examples/test_canny_pipeline.cpp src/core/image_processor.cpp)
target_link_libraries(test_canny_pipeline PRIVATE ${OpenCV_LIBS})
target_include_directories(test_canny_pipeline PRIVATE src src/core)

# Week 3.5 Filter Pipeline Test (Design Patterns)
add_executable(test_filter_pipeline examples/test_filter_pipeline.cpp ${FILTER_SOURCES})
target_link_libraries(test_filter_pipeline PRIVATE ${OpenCV_LIBS})
target_include_directories(test_filter_pipeline PRIVATE src src/core)

# Week 4 Performance Benchmark
add_executable(test_benchmark examples/test_benchmark.cpp ${FILTER_SOURCES} ${WEEK4_SOURCES})
target_link_libraries(test_benchmark PRIVATE ${OpenCV_LIBS})
target_include_directories(test_benchmark PRIVATE src src/core src/infra)

# Week 4 Atomic Integration Test
add_executable(test_atomic_integration examples/test_atomic_integration.cpp ${FILTER_SOURCES} ${WEEK4_SOURCES})
target_link_libraries(test_atomic_integration PRIVATE ${OpenCV_LIBS})
target_include_directories(test_atomic_integration PRIVATE src src/core src/infra)

# Week 4 Server JPG Test
add_executable(test_server_jpg examples/test_server_jpg.cpp ${FILTER_SOURCES} ${WEEK4_SOURCES})
target_link_libraries(test_server_jpg PRIVATE ${OpenCV_LIBS})
target_include_directories(test_server_jpg PRIVATE src src/core src/infra)

# Testing
enable_testing()

find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests 
    tests/test_main.cpp 
    tests/test_filters.cpp
    tests/test_thread_pool.cpp
    tests/test_atomic_writer.cpp
    src/core/image_processor.cpp 
    src/utils/Logger.cpp 
    ${FILTER_SOURCES}
    ${WEEK4_SOURCES}
)
target_link_libraries(unit_tests PRIVATE Crow::Crow asio::asio GTest::gtest_main Threads::Threads ${OpenCV_LIBS} spdlog::spdlog)
target_include_directories(unit_tests PRIVATE src src/core src/infra)

include(GoogleTest)
gtest_discover_tests(unit_tests)
