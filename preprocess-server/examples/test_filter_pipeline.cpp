/**
 * Week 3.5 Pipeline Test with Real Image
 * Tests the new Filter Pipeline system with IMG_1294.jpg
 */

#include "filter_pipeline.h"
#include "pipeline_factory.h"
#include <iostream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    std::string outputDir = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\processed\\pipeline_test";
    
    // Create output directory
    fs::create_directories(outputDir);
    
    std::cout << "=== Week 3.5 Filter Pipeline Test ===" << std::endl;
    std::cout << "Input: " << inputPath << std::endl;
    std::cout << "Output: " << outputDir << std::endl;
    std::cout << std::endl;
    
    // Load image
    cv::Mat input = cv::imread(inputPath);
    if (input.empty()) {
        std::cerr << "Failed to load image: " << inputPath << std::endl;
        return 1;
    }
    std::cout << "Input image: " << input.cols << "x" << input.rows << " (" << input.channels() << " channels)" << std::endl;
    
    // ==================== Test 1: Preprocess Pipeline ====================
    std::cout << "\n--- Test 1: Preprocess Pipeline (Week 2) ---" << std::endl;
    {
        auto pipeline = PipelineFactory::createPreprocessPipeline();
        std::cout << "Pipeline size: " << pipeline.size() << " filters" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = pipeline.execute(input);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Result: " << result.cols << "x" << result.rows << " (" << result.channels() << " channels)" << std::endl;
        std::cout << "Time: " << duration.count() << "ms" << std::endl;
        
        cv::imwrite(outputDir + "\\1_preprocess_pipeline.jpg", result);
        std::cout << "Saved: 1_preprocess_pipeline.jpg" << std::endl;
    }
    
    // ==================== Test 2: Sketch Pipeline ====================
    std::cout << "\n--- Test 2: Sketch Pipeline (Week 3) ---" << std::endl;
    {
        auto pipeline = PipelineFactory::createSketchPipeline();
        std::cout << "Pipeline size: " << pipeline.size() << " filters" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = pipeline.execute(input);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Result: " << result.cols << "x" << result.rows << " (" << result.channels() << " channels)" << std::endl;
        std::cout << "Time: " << duration.count() << "ms" << std::endl;
        
        cv::imwrite(outputDir + "\\2_sketch_pipeline.jpg", result);
        std::cout << "Saved: 2_sketch_pipeline.jpg" << std::endl;
    }
    
    // ==================== Test 3: Custom Pipeline ====================
    std::cout << "\n--- Test 3: Custom Pipeline (Dynamic Composition) ---" << std::endl;
    {
        FilterPipeline pipeline;
        pipeline.add(std::make_unique<ResizeFilter>(512))
                .add(std::make_unique<DenoiseFilter>())
                .add(std::make_unique<CannyFilter>(30, 100));  // Lower thresholds
        
        std::cout << "Pipeline size: " << pipeline.size() << " filters" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = pipeline.execute(input);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Result: " << result.cols << "x" << result.rows << " (" << result.channels() << " channels)" << std::endl;
        std::cout << "Time: " << duration.count() << "ms" << std::endl;
        
        cv::imwrite(outputDir + "\\3_custom_pipeline.jpg", result);
        std::cout << "Saved: 3_custom_pipeline.jpg" << std::endl;
    }
    
    // ==================== Test 4: Edge Detection Pipeline ====================
    std::cout << "\n--- Test 4: Edge Detection Pipeline ---" << std::endl;
    {
        auto pipeline = PipelineFactory::createEdgeDetectionPipeline();
        std::cout << "Pipeline size: " << pipeline.size() << " filters" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = pipeline.execute(input);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Result: " << result.cols << "x" << result.rows << " (" << result.channels() << " channels)" << std::endl;
        std::cout << "Time: " << duration.count() << "ms" << std::endl;
        
        cv::imwrite(outputDir + "\\4_edge_detection.jpg", result);
        std::cout << "Saved: 4_edge_detection.jpg" << std::endl;
    }
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    std::cout << "Output files saved to: " << outputDir << std::endl;
    
    return 0;
}
