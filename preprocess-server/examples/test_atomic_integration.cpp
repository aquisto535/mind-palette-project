/**
 * Week 4 Integration Test: AtomicWriter with Real Image (PPM Format)
 * Tests atomic writing of processed images using IMG_1294.jpg
 * Uses PPM format to bypass OpenCV codec issues in test environment
 */

#include "atomic_writer.h"
#include "pipeline_factory.h"
#include "thread_pool.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

// Helper to check file existence
bool fileExists(const std::string& path) {
    return fs::exists(path);
}

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    std::string outputDir = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\processed\\atomic_test";
    
    // Create output directory
    fs::create_directories(outputDir);
    
    std::cout << "=== Week 4 AtomicWriter Integration Test -- Static Build (JPG) ===" << std::endl;
    std::cout << "Input: " << inputPath << std::endl;
    std::cout << "Output: " << outputDir << std::endl;
    std::cout << std::endl;
    
    // Load image
    cv::Mat input = cv::imread(inputPath);
    if (input.empty()) {
        std::cerr << "Failed to load input image!" << std::endl;
        return 1;
    }
    std::cout << "Image loaded: " << input.cols << "x" << input.rows << std::endl;
    
    // Create pipelines
    auto preprocessPipeline = PipelineFactory::createPreprocessPipeline();
    auto sketchPipeline = PipelineFactory::createSketchPipeline();
    
    // Process images
    std::cout << "Processing images..." << std::endl;
    cv::Mat preprocessed = preprocessPipeline.execute(input);
    cv::Mat sketch = sketchPipeline.execute(input);
    
    // Write using AtomicFileWriter (JPG format via write)
    std::cout << "\n--- Testing Atomic Write (JPG) ---" << std::endl;
    
    // 1. Write Preprocessed Image
    std::string path1 = outputDir + "\\atomic_preprocess.jpg";
    std::cout << "Writing: " << path1 << "... ";
    if (AtomicFileWriter::write(preprocessed, path1)) {
        std::cout << "SUCCESS" << std::endl;
    } else {
        std::cerr << "FAILED" << std::endl;
    }
    
    // 2. Write Sketch Image
    std::string path2 = outputDir + "\\atomic_sketch.jpg";
    std::cout << "Writing: " << path2 << "... ";
    if (AtomicFileWriter::write(sketch, path2)) {
        std::cout << "SUCCESS" << std::endl;
    } else {
        std::cerr << "FAILED" << std::endl;
    }
    
    // 3. Concurrent Write Test (Thread Pool)
    std::cout << "\n--- Testing Concurrent Atomic Writes ---" << std::endl;
    ThreadPool pool(4);
    std::vector<std::string> concurrentPaths;
    
    for (int i = 0; i < 4; ++i) {
        std::string path = outputDir + "\\concurrent_" + std::to_string(i) + ".jpg";
        concurrentPaths.push_back(path);
        
        // Use sketch image for concurrency test
        pool.enqueue([sketch, path]() {
            if (AtomicFileWriter::write(sketch, path)) {
                std::cout << "Thread write success: " << fs::path(path).filename() << std::endl;
            } else {
                std::cerr << "Thread write failed: " << fs::path(path).filename() << std::endl;
            }
        });
    }
    
    pool.shutdown();
    
    // Verify files exist
    std::cout << "\n--- Verification ---" << std::endl;
    bool allExist = true;
    for (const auto& path : concurrentPaths) {
        if (!fs::exists(path)) {
            std::cerr << "Missing file: " << path << std::endl;
            allExist = false;
        }
    }
    
    if (allExist && fs::exists(path1) && fs::exists(path2)) {
        std::cout << "All files created successfully!" << std::endl;
    } else {
        std::cerr << "Some files are missing!" << std::endl;
        return 1;
    }
    
    return 0;
}
