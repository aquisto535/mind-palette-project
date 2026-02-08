#include "canny_filter.h"

CannyFilter::CannyFilter(double lowThreshold, double highThreshold)
    : lowThreshold_(lowThreshold), highThreshold_(highThreshold) {}

cv::Mat CannyFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Ensure input is grayscale
    cv::Mat gray = input;
    if (input.channels() > 1) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    
    cv::Mat edges;
    // apertureSize=3 (Sobel kernel size)
    // L2gradient=true for more accurate gradient magnitude
    cv::Canny(gray, edges, lowThreshold_, highThreshold_, 3, true);
    
    return edges;
}
