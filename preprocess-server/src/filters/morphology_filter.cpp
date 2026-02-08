#include "morphology_filter.h"

MorphologyFilter::MorphologyFilter(int kernelSize, int operation)
    : kernelSize_(kernelSize), operation_(operation) {}

cv::Mat MorphologyFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }
    
    // Create structuring element
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT, 
        cv::Size(kernelSize_, kernelSize_)
    );
    
    cv::Mat result;
    cv::morphologyEx(input, result, operation_, kernel);
    
    return result;
}
