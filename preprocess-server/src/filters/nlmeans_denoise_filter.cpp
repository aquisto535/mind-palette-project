#include "nlmeans_denoise_filter.h"

NlMeansDenoiseFilter::NlMeansDenoiseFilter(float h, int templateWindowSize, int searchWindowSize)
    : h_(h), templateWindowSize_(templateWindowSize), searchWindowSize_(searchWindowSize) {}

cv::Mat NlMeansDenoiseFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }

    cv::Mat result;
    if (input.channels() == 1) {
        cv::fastNlMeansDenoising(input, result, h_, templateWindowSize_, searchWindowSize_);
    } else {
        cv::fastNlMeansDenoisingColored(input, result, h_, h_, templateWindowSize_, searchWindowSize_);
    }
    return result;
}
