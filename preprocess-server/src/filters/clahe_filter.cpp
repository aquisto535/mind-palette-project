#include "clahe_filter.h"

ClaheFilter::ClaheFilter(double clipLimit, int tileGridSize)
    : clipLimit_(clipLimit), tileGridSize_(tileGridSize) {}

cv::Mat ClaheFilter::apply(const cv::Mat& input) const {
    if (input.empty()) {
        return cv::Mat();
    }

    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit_, cv::Size(tileGridSize_, tileGridSize_));
    cv::Mat result;
    clahe->apply(gray, result);
    return result;
}
