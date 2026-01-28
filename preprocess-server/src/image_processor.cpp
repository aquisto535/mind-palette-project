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
    input.copyTo(processed);
    
    // Step 1: Resize to 512x512
    cv::resize(processed, processed, cv::Size(kTargetSize, kTargetSize));
    
    // Step 2: Noise reduction (GaussianBlur + medianBlur)
    cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0); // 5x5 커널로 가우시안 블러 적용
    cv::medianBlur(processed, processed, 3); // 3x3 커널로 중간값 필터링
    
    // Step 3: Convert to grayscale for edge detection
    cv::Mat grayscale;
    cv::cvtColor(processed, grayscale, cv::COLOR_BGR2GRAY);
    
    return grayscale;
}

bool ImageProcessor::Save(const cv::Mat& image, const std::string& path) {
    if (image.empty()) {
        return false;
    }
    return cv::imwrite(path, image);
}
