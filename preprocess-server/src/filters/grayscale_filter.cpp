#include "grayscale_filter.h"

cv::Mat GrayscaleFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Already grayscale
    if (input.channels() == 1) {
        return input.clone();
    }
    
    cv::Mat result;
    cv::cvtColor(input, result, cv::COLOR_BGR2GRAY);
    return result;
}
