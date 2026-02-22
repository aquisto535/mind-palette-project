#include "denoise_filter.h"

DenoiseFilter::DenoiseFilter(int gaussianSize, int medianSize) 
    : gaussianSize_(gaussianSize), medianSize_(medianSize) {}

cv::Mat DenoiseFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    cv::Mat result;
    input.copyTo(result);
    
    // GaussianBlur for high-frequency noise
    cv::GaussianBlur(result, result, cv::Size(gaussianSize_, gaussianSize_), 0);
    
    // medianBlur for salt-and-pepper noise
    if (medianSize_ > 0) {
        cv::medianBlur(result, result, medianSize_);
    }
    
    return result;
}
