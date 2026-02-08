#include "binarize_filter.h"

BinarizeFilter::BinarizeFilter(int blockSize, int c)
    : blockSize_(blockSize), c_(c) {}

cv::Mat BinarizeFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Ensure input is grayscale
    cv::Mat gray = input;
    if (input.channels() > 1) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    
    cv::Mat result;
    // Adaptive thresholding: works better for varying lighting conditions
    // ADAPTIVE_THRESH_GAUSSIAN_C: weighted sum (Gaussian blur) of neighborhood
    // THRESH_BINARY_INV: dark objects on light background
    cv::adaptiveThreshold(gray, result, 255, 
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                          cv::THRESH_BINARY_INV, 
                          blockSize_, c_);
    
    return result;
}
