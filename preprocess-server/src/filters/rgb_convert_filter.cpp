#include "rgb_convert_filter.h"

cv::Mat RgbConvertFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Already 3-channel
    if (input.channels() == 3) {
        return input.clone();
    }
    
    cv::Mat result;
    cv::cvtColor(input, result, cv::COLOR_GRAY2BGR);
    return result;
}
