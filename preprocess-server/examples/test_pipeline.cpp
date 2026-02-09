#include "image_processor.h"
#include <iostream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\human.jpg";
    std::string outputDir = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\processed\\pipeline";
    
    // Create output directory
    fs::create_directories(outputDir);
    
    ImageProcessor processor;
    
    std::cout << "=== Week 3 Pipeline Test ===" << std::endl;
    std::cout << "Pipeline: Preprocess -> GrabCut -> Canny -> Morphology -> Binarize" << std::endl;
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
    
    // === Step 1: Preprocess (Resize + Denoise + Grayscale) ===
    auto start = std::chrono::high_resolution_clock::now();
    cv::Mat step1_gray = processor.Preprocess(original);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[1] Preprocess (512x512 grayscale): " << duration << "ms" << std::endl;
    processor.Save(step1_gray, outputDir + "\\1_preprocess_grayscale.jpg");
    
    // === Step 2: GrabCut Background Removal ===
    // Use resized color image for GrabCut (Letterbox to match Preprocess geometry)
    cv::Mat resizedColor = processor.ResizeKeepingAspectRatio(original, 512);
    
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step2_mask = processor.RemoveBackground(resizedColor, 1); // iterCount=1 for speed
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[2] GrabCut (iterCount=1): " << duration << "ms" << std::endl;
    processor.Save(step2_mask, outputDir + "\\2_grabcut_mask.jpg");
    
    // Apply mask to grayscale
    cv::Mat step2_foreground;
    step1_gray.copyTo(step2_foreground, step2_mask);
    processor.Save(step2_foreground, outputDir + "\\2b_grabcut_applied.jpg");
    
    // === Step 3: Canny Edge Detection ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step3_edges = processor.DetectEdges(step2_foreground, 50, 150);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int edgeCount = cv::countNonZero(step3_edges);
    std::cout << "[3] Canny Edge (50/150): " << duration << "ms, edges=" << edgeCount << "px" << std::endl;
    processor.Save(step3_edges, outputDir + "\\3_canny_edges.jpg");
    
    // === Step 4: Morphology Enhancement ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step4_enhanced = processor.EnhanceContours(step3_edges, 3);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int enhancedCount = cv::countNonZero(step4_enhanced);
    std::cout << "[4] Morphology (MORPH_CLOSE): " << duration << "ms, pixels=" << enhancedCount << "px" << std::endl;
    processor.Save(step4_enhanced, outputDir + "\\4_morphology_enhanced.jpg");
    
    // === Step 5: Binarization ===
    start = std::chrono::high_resolution_clock::now();
    cv::Mat step5_binary = processor.Binarize(step2_foreground);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[5] Binarize (Adaptive): " << duration << "ms" << std::endl;
    processor.Save(step5_binary, outputDir + "\\5_binarized.jpg");
    
    // === Total Time ===
    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();
    
    std::cout << "\n=== Pipeline Complete ===" << std::endl;
    std::cout << "Total time: " << totalDuration << "ms" << std::endl;
    std::cout << "\nOutput directory: " << outputDir << std::endl;
    std::cout << "\nGenerated files (in order):" << std::endl;
    std::cout << "  0_original.jpg           - Original input" << std::endl;
    std::cout << "  1_preprocess_grayscale.jpg - After Preprocess" << std::endl;
    std::cout << "  2_grabcut_mask.jpg       - GrabCut Mask" << std::endl;
    std::cout << "  2b_grabcut_applied.jpg   - Foreground (mask applied)" << std::endl;
    std::cout << "  3_canny_edges.jpg        - Canny Edges" << std::endl;
    std::cout << "  4_morphology_enhanced.jpg - After Morphology" << std::endl;
    std::cout << "  5_binarized.jpg          - Final Binary" << std::endl;
    
    return 0;
}
