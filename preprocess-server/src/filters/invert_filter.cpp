#include "invert_filter.h"

cv::Mat InvertFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    cv::Mat result;
    cv::bitwise_not(input, result);
    return result;
}
