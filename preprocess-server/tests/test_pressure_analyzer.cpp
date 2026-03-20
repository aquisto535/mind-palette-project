#include <gtest/gtest.h>
#include "analysis/pressure_analyzer.h"

// ==================== PressureAnalyzer Tests ====================

TEST(PressureAnalyzerTest, EmptyInput_ReturnsNegativeScore) {
    PressureAnalyzer analyzer;
    PressureResult result = analyzer.analyze(cv::Mat());
    EXPECT_LT(result.pressureScore, 0.0);
}

TEST(PressureAnalyzerTest, DarkRChannel_ReturnsHighPressureScore) {
    // 어두운 R채널 = 강한 필압 (score > 0.5)
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(20, 20, 20));  // BGR: R=20 (dark)
    PressureAnalyzer analyzer;
    PressureResult result = analyzer.analyze(img);
    EXPECT_GT(result.pressureScore, 0.5);
}

TEST(PressureAnalyzerTest, BrightRChannel_ReturnsLowPressureScore) {
    // 밝은 R채널 = 약한 필압 (score < 0.5)
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(240, 240, 240));  // BGR: R=240 (bright)
    PressureAnalyzer analyzer;
    PressureResult result = analyzer.analyze(img);
    EXPECT_LT(result.pressureScore, 0.5);
}

TEST(PressureAnalyzerTest, ValidInput_HasHistogram) {
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    PressureAnalyzer analyzer;
    PressureResult result = analyzer.analyze(img);
    EXPECT_EQ(result.histogram.size(), 256u);
}

TEST(PressureAnalyzerTest, ValidInput_MeanIntensityInRange) {
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(128, 128, 100));  // R=100
    PressureAnalyzer analyzer;
    PressureResult result = analyzer.analyze(img);
    EXPECT_GE(result.meanIntensity, 0.0);
    EXPECT_LE(result.meanIntensity, 255.0);
}

TEST(PressureAnalyzerTest, ScoreIsNormalized) {
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    PressureAnalyzer analyzer;
    PressureResult result = analyzer.analyze(img);
    EXPECT_GE(result.pressureScore, 0.0);
    EXPECT_LE(result.pressureScore, 1.0);
}
