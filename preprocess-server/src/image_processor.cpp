#include "image_processor.h"
#include <iostream>

ImageProcessor::ImageProcessor() {
}

cv::Mat ImageProcessor::Load(const std::string& path) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) {
        std::cerr << "Failed to load image: " << path << std::endl;
    }
    return img;
}

cv::Mat ImageProcessor::Preprocess(const cv::Mat& input) {
    if (input.empty()) {
        return cv::Mat();
    }
    
    cv::Mat processed;
    // TODO: Implement resize, denoise, grayscale
    input.copyTo(processed);
    
    // Resize to 512x512
    cv::resize(processed, processed, cv::Size(kTargetSize, kTargetSize));
    
    return processed;
}

bool ImageProcessor::Save(const cv::Mat& image, const std::string& path) {
    if (image.empty()) {
        return false;
    }
    return cv::imwrite(path, image);
}
