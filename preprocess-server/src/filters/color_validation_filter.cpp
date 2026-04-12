#include "filters/color_validation_filter.h"
#include "core/validation_exception.h"

#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

ColorValidationFilter::ColorValidationFilter(int satThreshold, double colorPixelRatio)
    : satThreshold_(satThreshold), colorPixelRatio_(colorPixelRatio) {}

double ColorValidationFilter::computeColorRatio(const cv::Mat& input) const {
    cv::Mat hsv;
    cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    const cv::Mat& saturation = channels[1]; // S 채널 (0~255)

    cv::Mat colorMask;
    cv::threshold(saturation, colorMask, satThreshold_, 255, cv::THRESH_BINARY);
    const double colorPixelCount = static_cast<double>(cv::countNonZero(colorMask));
    const double totalPixels     = static_cast<double>(input.rows) * input.cols;
    return colorPixelCount / totalPixels;
}

cv::Mat ColorValidationFilter::apply(const cv::Mat& input) const {
    if (input.empty()) return input.clone();

    // 1채널(그레이스케일)은 색상 정보가 없으므로 통과
    if (input.channels() == 1) return input.clone();

    const double ratio = computeColorRatio(input);

    spdlog::info("[ColorValidationFilter] color ratio = {:.1f}% (threshold = {:.1f}%)",
                 ratio * 100.0, colorPixelRatio_ * 100.0);

    if (ratio >= colorPixelRatio_) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << "Color image detected: "
            << (ratio * 100.0) << "% colored pixels exceed threshold of "
            << (colorPixelRatio_ * 100.0) << "%";
        throw ValidationException(oss.str());
    }

    return input.clone();
}
