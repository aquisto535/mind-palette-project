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
                                             int valueThreshold)
    : satThreshold_(satThreshold),
      colorPixelRatio_(colorPixelRatio),
      valueThreshold_(valueThreshold) {}

double ColorValidationFilter::computeColorRatio(const cv::Mat& input) const {
    cv::Mat hsv;
    cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    const cv::Mat& saturation = channels[1]; // S (0~255)
    const cv::Mat& value      = channels[2]; // V (0~255)

    // Step 1. 배경 마스킹 — 흰 종이/조명 반사(V>valueThreshold_ AND S<satThreshold_)는 제외.
    //         단, 고채도 영역은 V가 높아도 배경으로 간주하지 않는다(순색 컬러 이미지 방어).
    cv::Mat darkMask;        // V <= valueThreshold_
    cv::threshold(value, darkMask, valueThreshold_, 255, cv::THRESH_BINARY_INV);
    cv::Mat saturatedMask;   // S >= satThreshold_
    cv::threshold(saturation, saturatedMask, satThreshold_ - 1, 255, cv::THRESH_BINARY);
    cv::Mat foregroundMask;  // dark OR saturated → 결정적 픽셀
    cv::bitwise_or(darkMask, saturatedMask, foregroundMask);
    const double foregroundCount = static_cast<double>(cv::countNonZero(foregroundMask));

    if (foregroundCount <= 0.0) {
        // 배경만 있는 이미지(흰 종이) → 컬러 판정 불가, 통과시킴
        return 0.0;
    }

    // Step 2. 결정적 픽셀 중 S >= satThreshold_ 인 픽셀을 컬러로 카운트
    cv::Mat colorMask;
    cv::bitwise_and(saturatedMask, foregroundMask, colorMask);
    const double colorPixelCount = static_cast<double>(cv::countNonZero(colorMask));

    return colorPixelCount / foregroundCount;
}

cv::Mat ColorValidationFilter::apply(const cv::Mat& input) const {
    if (input.empty()) return input.clone();

    // 1채널(그레이스케일)은 색상 정보가 없으므로 통과
    if (input.channels() == 1) return input.clone();

    const double ratio = computeColorRatio(input);

    spdlog::info("[ColorValidationFilter] color ratio = {:.1f}% "
                 "(threshold = {:.1f}%, V<={}, S>={})",
                 ratio * 100.0, colorPixelRatio_ * 100.0,
                 valueThreshold_, satThreshold_);

    if (ratio >= colorPixelRatio_) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << "Color image detected: "
            << (ratio * 100.0) << "% colored pixels (S>=" << satThreshold_
            << ") among foreground (V<=" << valueThreshold_
            << ") exceed threshold of " << (colorPixelRatio_ * 100.0) << "%";
        throw ValidationException(oss.str());
    }

    return input.clone();
}
