#include "core/image_processor.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: visualize_pipeline <image_path>" << std::endl;
        return 1;
    }

    std::string imagePath = argv[1];
    ImageProcessor processor;
    
    std::cout << "Loading image: " << imagePath << std::endl;
    cv::Mat image = processor.Load(imagePath);
    if (image.empty()) {
        std::cerr << "Failed to load image." << std::endl;
        return 1;
    }

    std::cout << "Processing..." << std::endl;
    cv::Mat processed = processor.Preprocess(image);
    if (processed.empty()) {
        std::cerr << "Preprocessing failed." << std::endl;
        return 1;
    }

    std::cout << "Saving merged output..." << std::endl;
    // Assume run from project root, or modify path if needed.
    // Use absolute path logic if possible, but relative is fine if careful.
    std::string outputDir = "shared_volume/output";
    
    std::string mergedPath = outputDir + "/processed_merged.png";
    if (cv::imwrite(mergedPath, processed)) {
        std::cout << "Saved: " << mergedPath << std::endl;
    } else {
        std::cerr << "Failed to save merged output to " << mergedPath << std::endl;
    }

    std::cout << "Splitting channels..." << std::endl;
    std::vector<cv::Mat> channels;
    cv::split(processed, channels);

    // Channel order is BGR in OpenCV
    // B -> Distance
    // G -> Inverted Binary (Shape)
    // R -> Gray (Pressure)
    
    cv::imwrite(outputDir + "/processed_B_dist.png", channels[0]);
    cv::imwrite(outputDir + "/processed_G_binary.png", channels[1]);
    cv::imwrite(outputDir + "/processed_R_gray.png", channels[2]);

    std::cout << "Done. Check " << outputDir << std::endl;
    return 0;
}
