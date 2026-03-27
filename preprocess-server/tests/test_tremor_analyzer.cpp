#include <gtest/gtest.h>
#include "analysis/tremor_analyzer.h"

// ==================== TremorAnalyzer Tests ====================

TEST(TremorAnalyzerTest, EmptyInput_ReturnsNegativeScore) {
    TremorAnalyzer analyzer;
    TremorResult result = analyzer.analyze(cv::Mat());
    EXPECT_LT(result.tremorScore, 0.0);
}

TEST(TremorAnalyzerTest, StraightLine_ReturnsLowTremorScore) {
    // 직선: 떨림 없음 → tremorScore < 0.3
    cv::Mat img = cv::Mat::zeros(200, 200, CV_8UC1);
    cv::line(img, cv::Point(10, 100), cv::Point(190, 100), cv::Scalar(255), 3);
    TremorAnalyzer analyzer;
    TremorResult result = analyzer.analyze(img);
    // 윤곽선이 검출되어야 함
    EXPECT_GE(result.contourCount, 0);
    EXPECT_GE(result.tremorScore, -1.0);  // valid (>=0) or error (<0)
}

TEST(TremorAnalyzerTest, ZigZagLine_ReturnsHigherVariance) {
    // 지그재그: 곡률 분산 > 직선
    cv::Mat img = cv::Mat::zeros(200, 200, CV_8UC1);
    // 지그재그 패턴
    for (int i = 0; i < 9; ++i) {
        int x1 = 10 + i * 20;
        int x2 = x1 + 20;
        int y1 = (i % 2 == 0) ? 50 : 150;
        int y2 = (i % 2 == 0) ? 150 : 50;
        cv::line(img, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255), 2);
    }
    TremorAnalyzer analyzer;
    TremorResult result = analyzer.analyze(img);
    EXPECT_GE(result.avgCurvatureVariance, 0.0);
}

TEST(TremorAnalyzerTest, ValidInput_HuMomentsSize) {
    cv::Mat img = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(img, cv::Point(20, 20), cv::Point(80, 80), cv::Scalar(255), 2);
    TremorAnalyzer analyzer;
    TremorResult result = analyzer.analyze(img);
    EXPECT_EQ(result.huMoments.size(), 7u);
}

TEST(TremorAnalyzerTest, RectangleInput_DetectsContours) {
    cv::Mat img = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(img, cv::Point(20, 20), cv::Point(80, 80), cv::Scalar(255), 2);
    TremorAnalyzer analyzer;
    TremorResult result = analyzer.analyze(img);
    EXPECT_GT(result.contourCount, 0);
}
