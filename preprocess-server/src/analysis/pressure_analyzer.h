#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief PressureResult - 필압 분석 결과
 *
 * pressureScore = 1.0 - (meanIntensity / 255.0)
 * 어두운 R채널 = 강한 필압 = 높은 score
 * Returns pressureScore < 0 on invalid input.
 */
struct PressureResult {
    double pressureScore;           // [0, 1], 높을수록 강한 필압
    std::vector<float> histogram;   // R채널 히스토그램 [256]
    double meanIntensity;           // R채널 평균 밝기
    double stdIntensity;            // R채널 표준편차
};

/**
 * @brief PressureAnalyzer - BGR 이미지의 R채널 히스토그램으로 필압 분석
 */
class PressureAnalyzer {
public:
    PressureResult analyze(const cv::Mat& input) const;
};
