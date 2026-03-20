#include <gtest/gtest.h>
#include "utils/quality_metrics.h"

// ==================== QualityMetrics Tests ====================

TEST(QualityMetricsTest, ComputePSNR_IdenticalImages_ReturnsHighValue) {
    cv::Mat img = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    double psnr = QualityMetrics::computePSNR(img, img);
    EXPECT_GT(psnr, 50.0);
}

TEST(QualityMetricsTest, ComputePSNR_DifferentImages_ReturnsFiniteValue) {
    cv::Mat img1 = cv::Mat::ones(100, 100, CV_8UC1) * 100;
    cv::Mat img2 = cv::Mat::ones(100, 100, CV_8UC1) * 200;
    double psnr = QualityMetrics::computePSNR(img1, img2);
    EXPECT_TRUE(std::isfinite(psnr));
    EXPECT_LT(psnr, 50.0);
}

TEST(QualityMetricsTest, ComputePSNR_EmptyInput_ReturnsNegative) {
    double psnr = QualityMetrics::computePSNR(cv::Mat(), cv::Mat());
    EXPECT_LT(psnr, 0.0);
}

TEST(QualityMetricsTest, ComputeSSIM_IdenticalImages_ReturnsNearOne) {
    cv::Mat img = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    double ssim = QualityMetrics::computeSSIM(img, img);
    EXPECT_NEAR(ssim, 1.0, 0.01);
}

TEST(QualityMetricsTest, ComputeSSIM_DifferentImages_ReturnsBelowOne) {
    cv::Mat img1 = cv::Mat::ones(100, 100, CV_8UC1) * 50;
    cv::Mat img2 = cv::Mat::ones(100, 100, CV_8UC1) * 200;
    double ssim = QualityMetrics::computeSSIM(img1, img2);
    EXPECT_TRUE(std::isfinite(ssim));
    EXPECT_LT(ssim, 1.0);
}

TEST(QualityMetricsTest, ComputeSSIM_EmptyInput_ReturnsNegative) {
    double ssim = QualityMetrics::computeSSIM(cv::Mat(), cv::Mat());
    EXPECT_LT(ssim, 0.0);
}
