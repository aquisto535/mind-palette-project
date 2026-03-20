#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief TremorResult - 선 떨림 분석 결과
 *
 * tremorScore: 인접 3점 각도 기반 곡률 분산으로 계산 (높을수록 떨림 큼)
 * Returns tremorScore < 0 on invalid input.
 */
struct TremorResult {
    double tremorScore;             // [0, 1], 높을수록 강한 떨림
    int contourCount;               // 검출된 윤곽선 수
    double avgCurvatureVariance;    // 윤곽선 평균 곡률 분산
    std::vector<double> huMoments; // Hu Moments [7]
};

/**
 * @brief TremorAnalyzer - 이진 이미지의 윤곽선 곡률 분산으로 선 떨림 분석
 *
 * 인접 3점(p[i-1], p[i], p[i+1]) 사이의 각도를 계산하여
 * 그 분산이 클수록 떨림으로 판단.
 */
class TremorAnalyzer {
public:
    TremorResult analyze(const cv::Mat& input) const;

private:
    double computeCurvatureVariance(const std::vector<cv::Point>& contour) const;
};
