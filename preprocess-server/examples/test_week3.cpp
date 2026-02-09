#include "image_processor.h"
#include <iostream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string inputPath = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\human.jpg";
    std::string outputDir = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\processed";
    
    // Create output directory if not exists
    fs::create_directories(outputDir);
    
    ImageProcessor processor;
    
    std::cout << "=== Week 3 Image Processing Test ===" << std::endl;
    std::cout << "Input: " << inputPath << std::endl << std::endl;
    
    // Load image
    auto startTotal = std::chrono::high_resolution_clock::now();
    cv::Mat original = processor.Load(inputPath);
    if (original.empty()) {
        std::cerr << "Failed to load image!" << std::endl;
        return 1;
    }
    std::cout << "Image loaded: " << original.cols << "x" << original.rows << std::endl;
    
    // 1. Standard Preprocess (Resize + Denoise + Grayscale)
    auto start = std::chrono::high_resolution_clock::now();
    cv::Mat gray = processor.Preprocess(original);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[1] Preprocess (512x512 grayscale): " << duration << "ms" << std::endl;
    processor.Save(gray, outputDir + "\\human_1_grayscale.jpg");
    
    // 2. GrabCut Background Removal (iterCount comparison)
    std::cout << "\n[2] GrabCut Background Removal:" << std::endl;
    
    // iterCount = 1
    start = std::chrono::high_resolution_clock::now();
    cv::Mat mask1 = processor.RemoveBackground(original, 1);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    iterCount=1: " << duration << "ms" << std::endl;
    processor.Save(mask1, outputDir + "\\human_2a_grabcut_iter1.jpg");
    
    // iterCount = 3
    start = std::chrono::high_resolution_clock::now();
    cv::Mat mask3 = processor.RemoveBackground(original, 3);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    iterCount=3: " << duration << "ms" << std::endl;
    processor.Save(mask3, outputDir + "\\human_2b_grabcut_iter3.jpg");
    
    // iterCount = 5
    start = std::chrono::high_resolution_clock::now();
    cv::Mat mask5 = processor.RemoveBackground(original, 5);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    iterCount=5: " << duration << "ms" << std::endl;
    processor.Save(mask5, outputDir + "\\human_2c_grabcut_iter5.jpg");
    
    // Apply mask to original (show foreground only)
    cv::Mat foreground;
    original.copyTo(foreground, mask3);
    processor.Save(foreground, outputDir + "\\human_2d_foreground.jpg");
    
    // 3. Canny Edge Detection
    std::cout << "\n[3] Canny Edge Detection:" << std::endl;
    
    // Low threshold
    start = std::chrono::high_resolution_clock::now();
    cv::Mat edgesLow = processor.DetectEdges(gray, 30, 90);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    threshold=30/90: " << duration << "ms, edges=" << cv::countNonZero(edgesLow) << "px" << std::endl;
    processor.Save(edgesLow, outputDir + "\\human_3a_canny_low.jpg");
    
    // Default threshold
    start = std::chrono::high_resolution_clock::now();
    cv::Mat edgesMid = processor.DetectEdges(gray, 50, 150);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    threshold=50/150: " << duration << "ms, edges=" << cv::countNonZero(edgesMid) << "px" << std::endl;
    processor.Save(edgesMid, outputDir + "\\human_3b_canny_mid.jpg");
    
    // High threshold
    start = std::chrono::high_resolution_clock::now();
    cv::Mat edgesHigh = processor.DetectEdges(gray, 100, 200);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    threshold=100/200: " << duration << "ms, edges=" << cv::countNonZero(edgesHigh) << "px" << std::endl;
    processor.Save(edgesHigh, outputDir + "\\human_3c_canny_high.jpg");
    
    // 4. Morphology Enhancement
    std::cout << "\n[4] Morphology Enhancement (MORPH_CLOSE):" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    cv::Mat enhanced = processor.EnhanceContours(edgesMid, 3);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    kernelSize=3: " << duration << "ms" << std::endl;
    processor.Save(enhanced, outputDir + "\\human_4_enhanced.jpg");
    
    // 5. Binarization
    std::cout << "\n[5] Adaptive Binarization:" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    cv::Mat binary = processor.Binarize(gray);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "    adaptiveThreshold: " << duration << "ms" << std::endl;
    processor.Save(binary, outputDir + "\\human_5_binary.jpg");
    
    // Total time
    auto endTotal = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTotal - startTotal).count();
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Total processing time: " << totalDuration << "ms" << std::endl;
    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << "\nGenerated files:" << std::endl;
    std::cout << "  - human_1_grayscale.jpg" << std::endl;
    std::cout << "  - human_2a_grabcut_iter1.jpg" << std::endl;
    std::cout << "  - human_2b_grabcut_iter3.jpg" << std::endl;
    std::cout << "  - human_2c_grabcut_iter5.jpg" << std::endl;
    std::cout << "  - human_2d_foreground.jpg" << std::endl;
    std::cout << "  - human_3a_canny_low.jpg" << std::endl;
    std::cout << "  - human_3b_canny_mid.jpg" << std::endl;
    std::cout << "  - human_3c_canny_high.jpg" << std::endl;
    std::cout << "  - human_4_enhanced.jpg" << std::endl;
    std::cout << "  - human_5_binary.jpg" << std::endl;
    
    return 0;
}
