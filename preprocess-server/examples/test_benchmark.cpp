/**
 * Week 4 Performance Benchmark Test
 * Tests pipeline performance with real image and scalability
 */

#include "benchmark.h"
#include "pipeline_factory.h"
#include "thread_pool.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    
    std::cout << "=== Week 4 Performance Benchmark ===" << std::endl;
    std::cout << "Input: " << inputPath << std::endl;
    std::cout << std::endl;
    
    // Load test image
    cv::Mat input = cv::imread(inputPath);
    if (input.empty()) {
        std::cerr << "Failed to load test image!" << std::endl;
        return 1;
    }
    std::cout << "Image size: " << input.cols << "x" << input.rows << std::endl;
    
    // ==================== Test 1: Preprocess Pipeline (< 100ms target) ====================
    std::cout << "\n--- Test 1: Preprocess Pipeline Performance ---" << std::endl;
    {
        auto pipeline = PipelineFactory::createPreprocessPipeline();
        
        // Warm up
        pipeline.execute(input);
        
        // Measure average over 10 runs
        double avgTime = Benchmark::measureAverage([&]() {
            pipeline.execute(input);
        }, 10);
        
        std::cout << "Average time: " << Benchmark::formatDuration(avgTime) << std::endl;
        std::cout << "Target: < 100ms" << std::endl;
        std::cout << "Result: " << (avgTime < 100 ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // ==================== Test 2: Sketch Pipeline (< 100ms target) ====================
    std::cout << "\n--- Test 2: Sketch Pipeline Performance ---" << std::endl;
    {
        auto pipeline = PipelineFactory::createSketchPipeline();
        
        // Warm up
        pipeline.execute(input);
        
        // Measure average over 10 runs
        double avgTime = Benchmark::measureAverage([&]() {
            pipeline.execute(input);
        }, 10);
        
        std::cout << "Average time: " << Benchmark::formatDuration(avgTime) << std::endl;
        std::cout << "Target: < 100ms" << std::endl;
        std::cout << "Result: " << (avgTime < 100 ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // ==================== Test 3: ThreadPool Scalability ====================
    std::cout << "\n--- Test 3: ThreadPool Scalability ---" << std::endl;
    {
        auto pipeline = PipelineFactory::createSketchPipeline();
        
        Benchmark::runScalabilityTest(1, 8, [&]() {
            pipeline.execute(input);
        }, 20);
    }
    
    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    
    return 0;
}
