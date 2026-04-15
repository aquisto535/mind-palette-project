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

    // Step 1. HSV 채도 + RGB 채널 간 편차(chroma)를 함께 사용해 실제 채색 흔적 후보를 찾는다.
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

    cv::Mat colorCandidateMask;
    cv::bitwise_and(saturatedMask, chromaMask, colorCandidateMask);

    // Step 2. "밝고 비채색" 픽셀만 종이 배경으로 제외한다.
    // 기존의 V-only 마스킹은 밝은 빨강/파랑 스트로크까지 배경으로 버려서 회귀를 만들었다.
    cv::Mat brightMask; // V > valueThreshold_
    cv::threshold(value, brightMask, valueThreshold_, 255, cv::THRESH_BINARY);

    cv::Mat nonColorMask;
    cv::bitwise_not(colorCandidateMask, nonColorMask);

    cv::Mat paperMask;
    cv::bitwise_and(brightMask, nonColorMask, paperMask);

    cv::Mat foregroundMask;
    cv::bitwise_not(paperMask, foregroundMask);
    const double foregroundCount = static_cast<double>(cv::countNonZero(foregroundMask));

    if (foregroundCount <= 0.0) {
        return 0.0;
    }

    cv::Mat colorMask;
    cv::bitwise_and(colorCandidateMask, foregroundMask, colorMask);
    const double colorPixelCount = static_cast<double>(cv::countNonZero(colorMask));

    return colorPixelCount / foregroundCount;
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
            << ") among non-paper pixels (paper: V>" << valueThreshold_
            << " and not color candidate"
            << ") exceed threshold of " << (colorPixelRatio_ * 100.0) << "%";
        throw ValidationException(oss.str());
    }

    return input.clone();
}
