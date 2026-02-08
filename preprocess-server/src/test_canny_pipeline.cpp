// Test Canny-based pipeline (GrabCut excluded)
// Pipeline: Preprocess → Canny → Morphology → Binarize

#include "image_processor.h"
#include <iostream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    std::string outputDir = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\processed\\canny_pipeline_img1294";
    
    // Create output directory
    fs::create_directories(outputDir);
    
    ImageProcessor processor;
    
    std::cout << "=== Canny Pipeline Test (GrabCut Excluded) ===" << std::endl;
    std::cout << "Pipeline: Preprocess -> Canny -> Morphology -> Binarize" << std::endl;
    std::cout << "Input: " << inputPath << std::endl << std::endl;
    
    auto totalStart = std::chrono::high_resolution_clock::now();
    
    // === Step 0: Load Original ===
    cv::Mat original = processor.Load(inputPath);
    if (original.empty()) {
        std::cerr << "Failed to load image!" << std::endl;
        return 1;
    }
    std::cout << "[0] Original loaded: " << original.cols << "x" << original.rows << std::endl;
    processor.Save(original, outputDir + "\\0_original.jpg");
    
    // === Step 1: Preprocess (Letterbox + Denoise + Grayscale) ===
    auto start = std::chrono::high_resolution_clock::now();
    cv::Mat step1_preprocessed = processor.Preprocess(original);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[1] Preprocess (512x512 grayscale): " << duration << "ms" << std::endl;
    processor.Save(step1_preprocessed, outputDir + "\\1_preprocess.jpg");
    
    // === Step 2: Canny Edge Detection ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step2_edges = processor.DetectEdges(step1_preprocessed, 50, 150);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int edgeCount = cv::countNonZero(step2_edges);
    std::cout << "[2] Canny Edge (50/150): " << duration << "ms, edges=" << edgeCount << "px" << std::endl;
    processor.Save(step2_edges, outputDir + "\\2_canny_edges.jpg");
    
    // === Step 3: Morphology Enhancement ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step3_enhanced = processor.EnhanceContours(step2_edges, 3);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int enhancedCount = cv::countNonZero(step3_enhanced);
    std::cout << "[3] Morphology (MORPH_CLOSE): " << duration << "ms, pixels=" << enhancedCount << "px" << std::endl;
    processor.Save(step3_enhanced, outputDir + "\\3_morphology.jpg");
    
    // === Step 4: Binarization ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step4_binary = processor.Binarize(step1_preprocessed);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[4] Binarize (Adaptive): " << duration << "ms" << std::endl;
    processor.Save(step4_binary, outputDir + "\\4_binarized.jpg");
    
    // === Step 5: Convert to RGB 3-channel (for EfficientNet-B2) ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step5_rgb;
    cv::cvtColor(step4_binary, step5_rgb, cv::COLOR_GRAY2BGR);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[5] RGB Convert (3-channel): " << duration << "ms, channels=" << step5_rgb.channels() << std::endl;
    processor.Save(step5_rgb, outputDir + "\\5_final_rgb.jpg");
    
    // === Total Time ===
    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();
    
    std::cout << "\n=== Pipeline Complete ===" << std::endl;
    std::cout << "Total time: " << totalDuration << "ms" << std::endl;
    std::cout << "\nOutput directory: " << outputDir << std::endl;
    std::cout << "\nGenerated files:" << std::endl;
    std::cout << "  0_original.jpg    - Original input" << std::endl;
    std::cout << "  1_preprocess.jpg  - After Preprocess (Letterbox + Denoise + Gray)" << std::endl;
    std::cout << "  2_canny_edges.jpg - Canny Edge Detection" << std::endl;
    std::cout << "  3_morphology.jpg  - After Morphology Enhancement" << std::endl;
    std::cout << "  4_binarized.jpg   - Binarized (single channel)" << std::endl;
    std::cout << "  5_final_rgb.jpg   - *** FINAL OUTPUT (RGB 3-channel for AI) ***" << std::endl;
    
    return 0;
}

