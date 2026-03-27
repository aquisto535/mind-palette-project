#include "analysis/tremor_analyzer.h"
#include <cmath>
#include <numeric>

static constexpr double PI = 3.14159265358979323846;

double TremorAnalyzer::computeCurvatureVariance(const std::vector<cv::Point>& contour) const {
    const int n = static_cast<int>(contour.size());
    if (n < 3) return 0.0;

    std::vector<double> angles;
    angles.reserve(n);

    for (int i = 0; i < n; ++i) {
        const cv::Point& prev = contour[(i - 1 + n) % n];
        const cv::Point& curr = contour[i];
        const cv::Point& next = contour[(i + 1) % n];

        double dx1 = curr.x - prev.x;
        double dy1 = curr.y - prev.y;
        double dx2 = next.x - curr.x;
        double dy2 = next.y - curr.y;

        double len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
        double len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);

        if (len1 < 1e-9 || len2 < 1e-9) continue;

        double dot = (dx1 * dx2 + dy1 * dy2) / (len1 * len2);
        dot = std::max(-1.0, std::min(1.0, dot));  // clamp for acos
        double angle = std::acos(dot);
        angles.push_back(angle);
    }

    if (angles.empty()) return 0.0;

    double mean = std::accumulate(angles.begin(), angles.end(), 0.0) / angles.size();
    double variance = 0.0;
    for (double a : angles) {
        variance += (a - mean) * (a - mean);
    }
    variance /= angles.size();
    return variance;
}

TremorResult TremorAnalyzer::analyze(const cv::Mat& input) const {
    TremorResult result;
    result.tremorScore           = -1.0;
    result.contourCount          = 0;
    result.avgCurvatureVariance  = 0.0;
    result.huMoments.assign(7, 0.0);

    if (input.empty()) {
        return result;
    }

    // 이진화 (이미 이진 이미지라면 그대로 사용)
    cv::Mat binary;
    if (input.channels() == 3) {
        cv::cvtColor(input, binary, cv::COLOR_BGR2GRAY);
    } else {
        binary = input.clone();
    }
    if (binary.type() != CV_8UC1) {
        binary.convertTo(binary, CV_8UC1);
    }
    cv::threshold(binary, binary, 127, 255, cv::THRESH_BINARY);

    // 윤곽선 추출
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    result.contourCount = static_cast<int>(contours.size());

    // Hu Moments (전체 이미지 기반)
    cv::Moments m = cv::moments(binary);
    double hu[7];
    cv::HuMoments(m, hu);
    result.huMoments.assign(hu, hu + 7);

    if (contours.empty()) {
        result.tremorScore = 0.0;
        return result;
    }

    // 곡률 분산 평균 계산
    double totalVariance = 0.0;
    int validCount = 0;
    for (const auto& contour : contours) {
        if (contour.size() < 3) continue;
        totalVariance += computeCurvatureVariance(contour);
        ++validCount;
    }

    result.avgCurvatureVariance = (validCount > 0) ? (totalVariance / validCount) : 0.0;

    // tremorScore: variance를 [0,1]로 정규화 (PI^2/4 ≈ 2.47 가 최대 분산)
    const double maxVariance = (PI * PI) / 4.0;
    result.tremorScore = std::min(result.avgCurvatureVariance / maxVariance, 1.0);

    return result;
}
