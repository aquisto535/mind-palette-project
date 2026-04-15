#include "filters/color_validation_filter.h"
#include "core/validation_exception.h"

#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

ColorValidationFilter::ColorValidationFilter(int satThreshold,
                                             double colorPixelRatio,
                                             int valueThreshold,
                                             int chromaThreshold)
    : satThreshold_(satThreshold),
      colorPixelRatio_(colorPixelRatio),
      valueThreshold_(valueThreshold),
      chromaThreshold_(chromaThreshold) {}

double ColorValidationFilter::computeColorRatio(const cv::Mat& input) const {
    cv::Mat hsv;
    cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    const cv::Mat& saturation = channels[1]; // S (0~255)
    const cv::Mat& value      = channels[2]; // V (0~255)

    // Step 1. 실제 스트로크에 가까운 어두운 픽셀만 모수로 사용한다.
    // 배경 종이의 황변/조명 편차/JPEG 색번짐이 모수에 들어오면 정상 연필화가 오탐될 수 있다.
    cv::Mat strokeMask;      // V <= valueThreshold_
    cv::threshold(value, strokeMask, valueThreshold_, 255, cv::THRESH_BINARY_INV);
    const double strokeCount = static_cast<double>(cv::countNonZero(strokeMask));

    if (strokeCount <= 0.0) {
        return 0.0;
    }

    // Step 2. HSV 채도 + RGB 채널 간 편차(chroma)를 함께 사용해 실제 채색 흔적만 카운트한다.
    cv::Mat saturatedMask;   // S >= satThreshold_
    cv::threshold(saturation, saturatedMask, satThreshold_ - 1, 255, cv::THRESH_BINARY);
    std::vector<cv::Mat> bgrChannels;
    cv::split(input, bgrChannels);
    cv::Mat maxChannel, minChannel, chroma;
    cv::max(bgrChannels[0], bgrChannels[1], maxChannel);
    cv::max(maxChannel, bgrChannels[2], maxChannel);
    cv::min(bgrChannels[0], bgrChannels[1], minChannel);
    cv::min(minChannel, bgrChannels[2], minChannel);
    cv::subtract(maxChannel, minChannel, chroma);

    cv::Mat chromaMask;      // max-min >= chromaThreshold_
    cv::threshold(chroma, chromaMask, chromaThreshold_ - 1, 255, cv::THRESH_BINARY);

    cv::Mat candidateMask;
    cv::bitwise_and(saturatedMask, chromaMask, candidateMask);

    cv::Mat colorMask;
    cv::bitwise_and(candidateMask, strokeMask, colorMask);
    const double colorPixelCount = static_cast<double>(cv::countNonZero(colorMask));

    return colorPixelCount / strokeCount;
}

cv::Mat ColorValidationFilter::apply(const cv::Mat& input) const {
    if (input.empty()) return input.clone();

    // 1채널(그레이스케일)은 색상 정보가 없으므로 통과
    if (input.channels() == 1) return input.clone();

    const double ratio = computeColorRatio(input);

    spdlog::info("[ColorValidationFilter] color ratio = {:.1f}% "
                 "(threshold = {:.1f}%, V<={}, S>={}, chroma>={})",
                 ratio * 100.0, colorPixelRatio_ * 100.0,
                 valueThreshold_, satThreshold_, chromaThreshold_);

    if (ratio >= colorPixelRatio_) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << "Color stroke detected: "
            << (ratio * 100.0) << "% colored pixels (S>=" << satThreshold_
            << ", chroma>=" << chromaThreshold_
            << ") among stroke pixels (V<=" << valueThreshold_
            << ") exceed threshold of " << (colorPixelRatio_ * 100.0) << "%";
        throw ValidationException(oss.str());
    }

    return input.clone();
}
