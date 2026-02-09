/**
 * Week 4 Manual Test: Real Server Execution
 * Runs preprocess_server Logic directly to test JPG output
 */

#include "pipeline_factory.h"
#include "atomic_writer.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    std::string outputDir = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\processed\\server_test";
    
    fs::create_directories(outputDir);
    
    std::cout << "=== Real Server JPG Generation Test ===" << std::endl;
    std::cout << std::flush;
    
    // 1. Load Image
    std::cout << "Loading image from: " << inputPath << std::endl;
    std::cout << std::flush;
    
    cv::Mat input;
    try {
        input = cv::imread(inputPath);
    } catch (const std::exception& e) {
        std::cerr << "cv::imread exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "cv::imread unknown exception!" << std::endl;
        return 1;
    }
    
    if (input.empty()) {
        std::cerr << "Failed to load input image! (empty Mat)" << std::endl;
        return 1;
    }
    std::cout << "Input loaded: " << input.cols << "x" << input.rows << std::endl;
    
    // 2. Process (Preprocess Pipeline)
    std::cout << "Running Preprocess Pipeline..." << std::endl;
    auto pipeline = PipelineFactory::createPreprocessPipeline();
    cv::Mat result = pipeline.execute(input);
    
    // 3. Save as JPG (This is what the server does)
    std::string outputPath = outputDir + "\\server_output.jpg";
    std::cout << "Saving as JPG: " << outputPath << "... ";
    
    if (AtomicFileWriter::write(result, outputPath)) {
        std::cout << "SUCCESS" << std::endl;
        std::cout << "File size: " << fs::file_size(outputPath) << " bytes" << std::endl;
    } else {
        std::cerr << "FAILED" << std::endl;
        std::cerr << "OpenCV Codec Issue Confirmed if this fails." << std::endl;
    }
    
    return 0;
}
