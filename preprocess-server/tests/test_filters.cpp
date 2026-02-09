#include <gtest/gtest.h>
#include "filter.h"
#include "filter_pipeline.h"
#include "pipeline_factory.h"
#include "filters/resize_filter.h"
#include "filters/denoise_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/canny_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/rgb_convert_filter.h"
#include "filters/invert_filter.h"

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

TEST(FilterTest, CannyFilter_DetectsEdges) {
    cv::Mat input = cv::Mat::zeros(100, 100, CV_8UC1);
    // Draw a rectangle to create edges
    cv::rectangle(input, cv::Point(25, 25), cv::Point(75, 75), 255, -1);
    
    CannyFilter filter(50, 150);
    cv::Mat result = filter.apply(input);
    
    EXPECT_FALSE(result.empty());
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

TEST(PipelineFactoryTest, CreateSketchPipeline) {
    FilterPipeline pipeline = PipelineFactory::createSketchPipeline();
    
    EXPECT_EQ(pipeline.size(), 7);  // Resize, Denoise, Grayscale, Canny, Morph, Binary, RGB
    
    cv::Mat input = cv::Mat::ones(1000, 1000, CV_8UC3) * 128;
    cv::Mat result = pipeline.execute(input);
    
    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);
    EXPECT_EQ(result.channels(), 3);  // RGB output
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
