#include <gtest/gtest.h>
#include "filter.h"
#include "filter_pipeline.h"
#include "pipeline_factory.h"
#include "filters/resize_filter.h"
#include "filters/color_validation_filter.h"
#include "core/validation_exception.h"
#include "filters/denoise_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/rgb_convert_filter.h"
#include "filters/invert_filter.h"
#include "filters/clahe_filter.h"
#include "filters/nlmeans_denoise_filter.h"
#include "filters/otsu_canny_filter.h"
#include "filters/hybrid_preprocess_filter.h"
#include "core/image_processor.h"

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

    EXPECT_EQ(pipeline.size(), 4);  // Resize(768), ColorValidation, Denoise, HybridPreprocess

    // Use a neutral gray image (R=G=B=128) to pass color validation
    cv::Mat input(1000, 1000, CV_8UC3, cv::Scalar(128, 128, 128));
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

// ==================== GetContentROI (Dominant + Proximity) Tests ====================
// 부속 자극 제거 검증: 계획서 docs/project-guides/preprocess_roi_accessory_removal_plan.md 참고

static cv::Mat makeBinaryWithRects(int rows, int cols,
                                   const std::vector<cv::Rect>& rects) {
    cv::Mat img = cv::Mat::zeros(rows, cols, CV_8UC1);
    for (const auto& r : rects) {
        cv::rectangle(img, r, cv::Scalar(255), cv::FILLED);
    }
    return img;
}

// Case 1: 메인 인물 + 멀리 떨어진 부속 자극 → 부속 자극 제외
//   main=(100,100,200,200), accessory=(430,430,40,40)
//   dominantRect=main, margin=20% → expandedRect 우단 ≈ 340 < 430 → 제외
TEST(GetContentROITest, ExcludesDistantAccessories) {
    cv::Mat binary = makeBinaryWithRects(500, 500, {
        cv::Rect(100, 100, 200, 200),  // 메인 인물 (40000px²)
        cv::Rect(430, 430,  40,  40),  // 멀리 떨어진 부속 자극 (1600px²)
    });

    ImageProcessor proc;
    cv::Rect roi = proc.GetContentROI(binary);

    // 부속 자극(x=430)이 ROI에 포함되어선 안 됨
    EXPECT_LT(roi.x + roi.width, 430)
        << "ROI should not include distant accessory. roi=("
        << roi.x << "," << roi.y << "," << roi.width << "," << roi.height << ")";
}

// Case 2: 메인 인물 + 근접한 분리 선(팔/다리) → Dominant만 반환 (근접 선 제외)
//   Dominant only 방식: 가장 큰 컨투어의 bbox만 반환
//   main=(100,100,200,200), nearby=(320,100,30,200)
//   ROI 우단 < 320 (nearby 제외)
TEST(GetContentROITest, DominantOnly_ExcludesNearbyFragment) {
    cv::Mat binary = makeBinaryWithRects(500, 500, {
        cv::Rect(100, 100, 200, 200),  // 메인 인물 (40000px²)
        cv::Rect(320, 100,  30, 200),  // 근접한 분리 선 (6000px²) — Dominant only이므로 제외
    });

    ImageProcessor proc;
    cv::Rect roi = proc.GetContentROI(binary);

    // Dominant(메인 인물)만 포함 — 우단이 320 미만
    EXPECT_LE(roi.x, 100)
        << "ROI left edge should cover main figure";
    EXPECT_LT(roi.x + roi.width, 320)
        << "ROI should not include nearby fragment in dominant-only mode. roi=("
        << roi.x << "," << roi.y << "," << roi.width << "," << roi.height << ")";
}

// Case 3: 단일 컨투어 → 기존 동작 유지 (bounding rect + padding)
TEST(GetContentROITest, SingleContour_ReturnsBoundingRectWithPadding) {
    cv::Mat binary = makeBinaryWithRects(500, 500, {
        cv::Rect(100, 100, 200, 200),
    });

    ImageProcessor proc;
    cv::Rect roi = proc.GetContentROI(binary);

    // 단일 컨투어의 bounding rect를 포함해야 함 (padding 포함)
    EXPECT_LE(roi.x, 100);
    EXPECT_LE(roi.y, 100);
    EXPECT_GE(roi.x + roi.width,  300);  // 100+200
    EXPECT_GE(roi.y + roi.height, 300);
}

// Case 4: 빈 이미지 → 전체 이미지 크기 fallback
TEST(GetContentROITest, EmptyBinary_ReturnsFallback) {
    cv::Mat binary = cv::Mat::zeros(500, 500, CV_8UC1);

    ImageProcessor proc;
    cv::Rect roi = proc.GetContentROI(binary);

    EXPECT_EQ(roi.width,  500);
    EXPECT_EQ(roi.height, 500);
}

// ==================== ColorValidationFilter Tests (ADR-033 Tier 1) ====================

// 1. 1채널 그레이스케일 이미지 → 예외 없이 통과
TEST(ColorValidationFilterTest, PassesGrayscaleImage) {
    cv::Mat input = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    ColorValidationFilter filter;

    EXPECT_NO_THROW(filter.apply(input));
    cv::Mat result = filter.apply(input);
    EXPECT_FALSE(result.empty());
}

// 2. 낮은 채도의 BGR 이미지 (연필 그림 시뮬레이션) → 예외 없이 통과
TEST(ColorValidationFilterTest, PassesLowSaturationImage) {
    // 모든 픽셀을 R=G=B=128 (무채색)로 설정
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    ColorValidationFilter filter;

    EXPECT_NO_THROW(filter.apply(input));
}

// 3. 고채도 순수 빨강 이미지 → ValidationException throw
TEST(ColorValidationFilterTest, ThrowsOnColorImage) {
    // 순수 빨강: HSV S값이 최대(255)
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));  // BGR: 빨강
    ColorValidationFilter filter;

    EXPECT_THROW(filter.apply(input), ValidationException);
}

// 4. 5% 이상 컬러 픽셀 혼합 이미지 → ValidationException throw
TEST(ColorValidationFilterTest, ThrowsOnMixedColorImage) {
    // 회색 전경(V=128)에 10%를 빨간 픽셀로 채움
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    int colorRows = 10;  // 10행 = 10% 픽셀이 컬러
    input(cv::Rect(0, 0, 100, colorRows)) = cv::Scalar(0, 0, 255);
    ColorValidationFilter filter;  // 기본값: S>=50, V<=220, 5%

    EXPECT_THROW(filter.apply(input), ValidationException);
}

// 5. 5% 미만 컬러 픽셀 → 예외 없이 통과 (경계값 테스트)
TEST(ColorValidationFilterTest, PassesBorderlineImage) {
    // 회색 배경에 3%만 빨간 픽셀 (임계값 5% 미만)
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    int colorRows = 3;  // 3행 = 3%
    input(cv::Rect(0, 0, 100, colorRows)) = cv::Scalar(0, 0, 255);
    ColorValidationFilter filter;

    EXPECT_NO_THROW(filter.apply(input));
}

// 7. [ADR-033] 흰 종이(V=255) 전체 이미지 → 배경 제외, 통과
TEST(ColorValidationFilterTest, PassesWhitePaperOnly) {
    // 흰 종이만 있는 이미지 — 모두 V=255로 배경 처리, 결정적 픽셀 0 → 통과
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(255, 255, 255));
    ColorValidationFilter filter;

    EXPECT_NO_THROW(filter.apply(input));
}

// 8. [ADR-033] 흰 종이 위 연필 스트로크(V 낮음, S 낮음) → 통과
TEST(ColorValidationFilterTest, PassesPencilStrokeOnWhitePaper) {
    // 전체 흰 종이(V=255) 배경에 15%의 검정 스트로크(V=50, S=0) — 전형적인 연필 그림.
    // 결정적 픽셀 = 스트로크 15%, 컬러 픽셀 = 0 → ratio=0% → 통과
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(255, 255, 255));
    input(cv::Rect(0, 0, 100, 15)) = cv::Scalar(50, 50, 50);
    ColorValidationFilter filter;

    EXPECT_NO_THROW(filter.apply(input));
}

// 9. [ADR-033] 흰 종이 위 크레파스 컬러 스트로크 → ValidationException throw
TEST(ColorValidationFilterTest, ThrowsOnCrayonStrokeOnWhitePaper) {
    // 전체 흰 종이 배경(90%)에 10% 고채도 파랑 스트로크 — 컬러 드로잉.
    // 결정적 픽셀 = 파랑 10%, 컬러 픽셀 = 100% of foreground → 100% ≥ 5% → throw
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(255, 255, 255));
    input(cv::Rect(0, 0, 100, 10)) = cv::Scalar(255, 0, 0); // BGR: 파랑
    ColorValidationFilter filter;

    EXPECT_THROW(filter.apply(input), ValidationException);
}

// 10. [ADR-033] 낮은 채도 노이즈(S<50)는 무시 → 흑연 난반사 허용
TEST(ColorValidationFilterTest, IgnoresLowSaturationNoise) {
    // 회색 배경에 S=40 수준의 약한 색상 노이즈가 30% — 연필 그림의 황변/반사 시뮬.
    // satThreshold_=50이므로 S=40은 컬러로 판정되지 않음 → 통과
    cv::Mat input(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    // BGR (108, 128, 128) → HSV 계산 시 S ≈ 40 (약한 청록 틴트)
    input(cv::Rect(0, 0, 100, 30)) = cv::Scalar(108, 128, 128);
    ColorValidationFilter filter;

    EXPECT_NO_THROW(filter.apply(input));
}

// 6. 파이프라인 통합: createHybridPipeline + 컬러 이미지 → ValidationException 전파
TEST(ColorValidationFilterTest, PipelineThrowsForColorImage) {
    cv::Mat input(768, 768, CV_8UC3, cv::Scalar(0, 0, 255));  // 전체 빨강
    auto pipeline = PipelineFactory::createHybridPipeline();

    EXPECT_THROW(pipeline.execute(input), ValidationException);
}
