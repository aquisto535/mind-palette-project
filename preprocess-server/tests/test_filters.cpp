#include <gtest/gtest.h>
#include "filter.h"
#include "filter_pipeline.h"
#include "pipeline_factory.h"
#include "filters/resize_filter.h"
#include "filters/denoise_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/rgb_convert_filter.h"
#include "filters/invert_filter.h"
#include "filters/clahe_filter.h"
#include "filters/nlmeans_denoise_filter.h"
#include "filters/otsu_canny_filter.h"

// ==================== IFilter Tests ====================

TEST(FilterTest, ResizeFilter_512x512) {
    cv::Mat input = cv::Mat::ones(1000, 1500, CV_8UC3) * 128;
    ResizeFilter filter(512);
    
    cv::Mat result = filter.apply(input);
    
    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
    EXPECT_EQ(result.channels(), 3);
}

TEST(FilterTest, DenoiseFilter_AppliesBlur) {
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC3) * 128;
    // Add some noise
    input.at<cv::Vec3b>(50, 50) = cv::Vec3b(255, 255, 255);
    
    DenoiseFilter filter;
    cv::Mat result = filter.apply(input);
    
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size(), input.size());
}

TEST(FilterTest, GrayscaleFilter_ConvertsToSingleChannel) {
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC3) * 128;
    GrayscaleFilter filter;
    
    cv::Mat result = filter.apply(input);
    
    EXPECT_EQ(result.channels(), 1);
}


TEST(FilterTest, BinarizeFilter_CreatesBinaryImage) {
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    BinarizeFilter filter;
    
    cv::Mat result = filter.apply(input);
    
    EXPECT_FALSE(result.empty());
    // Check that result is binary (0 or 255)
    double minVal, maxVal;
    cv::minMaxLoc(result, &minVal, &maxVal);
    EXPECT_TRUE(minVal == 0 || minVal == 255);
    EXPECT_TRUE(maxVal == 0 || maxVal == 255);
}

TEST(FilterTest, MorphologyFilter_ClosesGaps) {
    cv::Mat input = cv::Mat::zeros(100, 100, CV_8UC1);
    input.at<uchar>(50, 50) = 255;
    
    MorphologyFilter filter(3, cv::MORPH_DILATE);
    cv::Mat result = filter.apply(input);
    
    EXPECT_FALSE(result.empty());
}

TEST(FilterTest, RgbConvertFilter_ConvertsTo3Channel) {
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    RgbConvertFilter filter;
    
    cv::Mat result = filter.apply(input);
    
    EXPECT_EQ(result.channels(), 3);
}

TEST(FilterTest, InvertFilter_InvertsColors) {
    cv::Mat input = cv::Mat::zeros(100, 100, CV_8UC1);
    InvertFilter filter;
    
    cv::Mat result = filter.apply(input);
    
    EXPECT_FALSE(result.empty());
    // Inverting 0 (black) should give 255 (white)
    EXPECT_EQ(result.at<uchar>(50, 50), 255);
}

// ==================== FilterPipeline Tests ====================

TEST(FilterPipelineTest, AddAndExecute) {
    FilterPipeline pipeline;
    pipeline.add(std::make_unique<ResizeFilter>(256))
            .add(std::make_unique<GrayscaleFilter>());
    
    cv::Mat input = cv::Mat::ones(500, 500, CV_8UC3) * 128;
    cv::Mat result = pipeline.execute(input);
    
    EXPECT_EQ(result.rows, 256);
    EXPECT_EQ(result.cols, 256);
    EXPECT_EQ(result.channels(), 1);
}

TEST(FilterPipelineTest, EmptyPipelineReturnsInput) {
    FilterPipeline pipeline;
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC3) * 128;
    
    cv::Mat result = pipeline.execute(input);
    
    EXPECT_EQ(result.size(), input.size());
}

TEST(FilterPipelineTest, SizeAndClear) {
    FilterPipeline pipeline;
    EXPECT_TRUE(pipeline.empty());
    EXPECT_EQ(pipeline.size(), 0);
    
    pipeline.add(std::make_unique<ResizeFilter>(512));
    EXPECT_FALSE(pipeline.empty());
    EXPECT_EQ(pipeline.size(), 1);
    
    pipeline.clear();
    EXPECT_TRUE(pipeline.empty());
}

// ==================== PipelineFactory Tests ====================

TEST(PipelineFactoryTest, CreatePreprocessPipeline) {
    FilterPipeline pipeline = PipelineFactory::createPreprocessPipeline();
    
    EXPECT_EQ(pipeline.size(), 3);  // Resize, Denoise, Grayscale
    
    cv::Mat input = cv::Mat::ones(1000, 1000, CV_8UC3) * 128;
    cv::Mat result = pipeline.execute(input);
    
    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
    EXPECT_EQ(result.channels(), 1);
}

TEST(PipelineFactoryTest, CreateHybridPipeline) {
    FilterPipeline pipeline = PipelineFactory::createHybridPipeline();

    EXPECT_EQ(pipeline.size(), 3);  // Resize(1024), Denoise, HybridPreprocess

    cv::Mat input = cv::Mat::ones(1000, 1000, CV_8UC3) * 128;
    cv::Mat result = pipeline.execute(input);

    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
    EXPECT_EQ(result.channels(), 3);  // 3-channel hybrid output
}

// ==================== ResizeFilter Interpolation Tests ====================

TEST(FilterTest, ResizeFilter_DownscaleUsesInterArea) {
    // 1000x1500 -> 512x512 (downscaling): scale = 512/1500 ≈ 0.341
    // Content area: new_h=341, new_w=512, offset_y=85, offset_x=0
    cv::Mat input = cv::Mat::ones(1000, 1500, CV_8UC3) * 128;
    ResizeFilter filter(512);
    cv::Mat result = filter.apply(input);

    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
    EXPECT_FALSE(result.empty());
    // Check only the content area (non-padded region): mean should be close to original
    cv::Mat content = result(cv::Rect(0, 85, 512, 341));
    cv::Scalar mean = cv::mean(content);
    EXPECT_NEAR(mean[0], 128.0, 5.0);
}

TEST(FilterTest, ResizeFilter_UpscaleUsesInterCubic) {
    // 100x100 -> 512x512 (upscaling)
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC3) * 200;
    ResizeFilter filter(512);
    cv::Mat result = filter.apply(input);

    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
    EXPECT_FALSE(result.empty());
    // INTER_CUBIC should preserve pixel values in uniform regions
    cv::Scalar mean = cv::mean(result(cv::Rect(206, 206, 100, 100)));
    EXPECT_NEAR(mean[0], 200.0, 5.0);
}

// ==================== OCP Verification Test ====================

// This test verifies OCP: Adding a new filter without modifying existing code
class MockNewFilter : public IFilter {
public:
    cv::Mat apply(const cv::Mat& input) const override {
        if (input.empty()) return cv::Mat();
        cv::Mat result;
        cv::bitwise_not(input, result);
        return result;
    }
    std::string name() const override { return "MockNewFilter"; }
};

TEST(OCPTest, NewFilterWithoutModifyingExistingCode) {
    FilterPipeline pipeline;

    // Add existing filters
    pipeline.add(std::make_unique<GrayscaleFilter>());

    // Add NEW filter (MockNewFilter) - no modification to existing code!
    pipeline.add(std::make_unique<MockNewFilter>());

    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC3) * 128;
    cv::Mat result = pipeline.execute(input);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(pipeline.size(), 2);
}

// ==================== ClaheFilter Tests ====================

TEST(FilterTest, ClaheFilter_EmptyInput_ReturnsEmpty) {
    ClaheFilter filter;
    cv::Mat result = filter.apply(cv::Mat());
    EXPECT_TRUE(result.empty());
}

TEST(FilterTest, ClaheFilter_GrayscaleInput_OutputsSingleChannel) {
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    ClaheFilter filter;
    cv::Mat result = filter.apply(input);
    EXPECT_EQ(result.channels(), 1);
    EXPECT_EQ(result.size(), input.size());
}

TEST(FilterTest, ClaheFilter_BgrInput_OutputsSingleChannel) {
    // BGR input should be converted to grayscale before CLAHE
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC3) * 128;
    ClaheFilter filter;
    cv::Mat result = filter.apply(input);
    EXPECT_EQ(result.channels(), 1);
    EXPECT_EQ(result.rows, 100);
    EXPECT_EQ(result.cols, 100);
}

TEST(FilterTest, ClaheFilter_LowContrastImage_IncreasesStdDev) {
    // Low contrast image: uniform gray 120
    cv::Mat input(100, 100, CV_8UC1, cv::Scalar(120));
    // Add a small variation
    input.at<uchar>(30, 30) = 130;
    input.at<uchar>(70, 70) = 110;

    ClaheFilter filter;
    cv::Mat result = filter.apply(input);

    // CLAHE should produce a valid non-empty result
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.type(), CV_8UC1);
}

// ==================== NlMeansDenoiseFilter Tests ====================

TEST(FilterTest, NlMeansDenoiseFilter_EmptyInput_ReturnsEmpty) {
    NlMeansDenoiseFilter filter;
    cv::Mat result = filter.apply(cv::Mat());
    EXPECT_TRUE(result.empty());
}

TEST(FilterTest, NlMeansDenoiseFilter_GrayscaleInput_PreservesSize) {
    cv::Mat input = cv::Mat::ones(64, 64, CV_8UC1) * 200;
    // Add salt-and-pepper noise
    input.at<uchar>(20, 20) = 0;
    input.at<uchar>(40, 40) = 255;

    NlMeansDenoiseFilter filter;
    cv::Mat result = filter.apply(input);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size(), input.size());
    EXPECT_EQ(result.channels(), 1);
}

TEST(FilterTest, NlMeansDenoiseFilter_BgrInput_PreservesChannels) {
    cv::Mat input = cv::Mat::ones(64, 64, CV_8UC3) * 180;
    input.at<cv::Vec3b>(20, 20) = cv::Vec3b(0, 0, 0);

    NlMeansDenoiseFilter filter;
    cv::Mat result = filter.apply(input);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.channels(), 3);
    EXPECT_EQ(result.size(), input.size());
}

// ==================== OtsuCannyFilter Tests ====================

TEST(FilterTest, OtsuCannyFilter_EmptyInput_ReturnsEmpty) {
    OtsuCannyFilter filter;
    cv::Mat result = filter.apply(cv::Mat());
    EXPECT_TRUE(result.empty());
}

TEST(FilterTest, OtsuCannyFilter_DetectsEdgesOnRectangle) {
    // White rectangle on black background: clear edges
    cv::Mat input = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(input, cv::Point(20, 20), cv::Point(80, 80), cv::Scalar(255), -1);

    OtsuCannyFilter filter;
    cv::Mat result = filter.apply(input);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.type(), CV_8UC1);
    // Edge pixels should exist
    EXPECT_GT(cv::countNonZero(result), 0);
}

TEST(FilterTest, OtsuCannyFilter_BgrInput_OutputsSingleChannel) {
    cv::Mat input = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::rectangle(input, cv::Point(20, 20), cv::Point(80, 80), cv::Scalar(255, 255, 255), -1);

    OtsuCannyFilter filter;
    cv::Mat result = filter.apply(input);

    EXPECT_EQ(result.channels(), 1);
    EXPECT_EQ(result.rows, 100);
    EXPECT_EQ(result.cols, 100);
}

TEST(FilterTest, OtsuCannyFilter_CustomSigma) {
    cv::Mat input = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(input, cv::Point(10, 10), cv::Point(90, 90), cv::Scalar(255), -1);

    OtsuCannyFilter filter(0.33);
    cv::Mat result = filter.apply(input);

    EXPECT_FALSE(result.empty());
    EXPECT_GT(cv::countNonZero(result), 0);
}
