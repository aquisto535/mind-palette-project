#include "core/image_processor.h"
#include <iostream>
#include <algorithm> // for std::max
#include "filters/resize_filter.h"
#include "filters/denoise_filter.h"
#include "filters/grayscale_filter.h"
#include "filters/binarize_filter.h"
#include "filters/morphology_filter.h"
#include "filters/invert_filter.h"
#include "filters/rgb_convert_filter.h"
#include "filters/hybrid_preprocess_filter.h"
#include "core/pipeline_factory.h"
#include "utils/Logger.h"

ImageProcessor::ImageProcessor() {
}

cv::Mat ImageProcessor::Load(const std::string& path, const std::string& requestId) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) {
       LOG_ERROR(requestId, "Failed to load image: {}", path);
    }
    return img;
}

cv::Mat ImageProcessor::Preprocess(const cv::Mat& input, const std::string& requestId) {
    if (input.empty()) return cv::Mat();

    auto pipeline = PipelineFactory::createHybridPipeline();
    return pipeline.execute(input);
}

cv::Mat ImageProcessor::NormalizeGrayscale(const cv::Mat& input) {
    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
    return blurred;
}

cv::Mat ImageProcessor::ApplyAdaptiveBinarization(const cv::Mat& input) {
    // 냄새: 이 로직은 BinarizeFilter와 MorphologyFilter에 중복되어 있음.
    // 리팩토링: 기존 필터를 재사용하여 일관성 유지.
    auto binary = Binarize(input);
    return EnhanceContours(binary, 3);
}

ImageProcessor::ResizeResult ImageProcessor::ApplyLetterboxWithMetrics(const cv::Mat& input, int targetSize, int interpolation, uint8_t padValue) {
    float scale = static_cast<float>(targetSize) / std::max(input.cols, input.rows);
    int new_w = static_cast<int>(input.cols * scale);
    int new_h = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(new_w, new_h), 0, 0, interpolation);

    cv::Mat canvas(targetSize, targetSize, CV_8UC1, cv::Scalar(padValue));
    int x_offset = (targetSize - new_w) / 2;
    int y_offset = (targetSize - new_h) / 2;
    resized.copyTo(canvas(cv::Rect(x_offset, y_offset, new_w, new_h)));

    return {canvas, x_offset, y_offset, new_w, new_h};
}

cv::Mat ImageProcessor::GenerateDistanceMap(const cv::Mat& binary) {
    // binary: 검은 배경 / 흰 선 (resized_binary)
    cv::Mat dist, dist_8u;
    cv::distanceTransform(binary, dist, cv::DIST_L2, 5);
    cv::normalize(dist, dist, 0, 1.0, cv::NORM_MINMAX);
    dist = dist * 255;
    dist.convertTo(dist_8u, CV_8U);
    
    // 최종 결과물 스타일(흰 배경)에 맞추기 위해 반전
    cv::Mat inverted;
    cv::bitwise_not(dist_8u, inverted);
    return inverted;
}

bool ImageProcessor::Save(const cv::Mat& image, const std::string& path, const std::string& requestId) {
    if (image.empty()) {
        LOG_ERROR(requestId, "Attempted to save empty image to {}", path);
        return false;
    }
    return cv::imwrite(path, image);
}

// === Public APIs (Legacy Support / Granularity) ===

cv::Mat ImageProcessor::DetectEdges(const cv::Mat& grayscale, double lowThreshold, double highThreshold) {
    if (grayscale.empty()) return cv::Mat();
    cv::Mat input = (grayscale.channels() > 1) ? NormalizeGrayscale(grayscale) : grayscale;
    cv::Mat edges;
    cv::Canny(input, edges, lowThreshold, highThreshold, 3, true);
    return edges;
}

cv::Mat ImageProcessor::EnhanceContours(const cv::Mat& binary, int kernelSize) {
    if (binary.empty()) return cv::Mat();
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));
    cv::Mat result;
    cv::morphologyEx(binary, result, cv::MORPH_CLOSE, kernel);
    return result;
}

cv::Mat ImageProcessor::Binarize(const cv::Mat& grayscale) {
    if (grayscale.empty()) return cv::Mat();
    cv::Mat result;
    cv::adaptiveThreshold(grayscale, result, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);
    return result;
}

cv::Mat ImageProcessor::ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize) {
    return ApplyLetterboxWithMetrics(input, targetSize, cv::INTER_LINEAR, 0).canvas;
}

cv::Rect ImageProcessor::GetContentROI(const cv::Mat& binary) {
    if (binary.empty()) return cv::Rect(0, 0, 0, 0);

    cv::Mat morph;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(binary, morph, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(morph, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) return cv::Rect(0, 0, binary.cols, binary.rows);

    double totalArea = binary.cols * binary.rows;

    // Step 1: 면적 0.1% 이상인 유효 컨투어 수집
    std::vector<std::pair<double, cv::Rect>> validRects;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < totalArea * 0.001) continue;
        validRects.push_back({area, cv::boundingRect(contour)});
    }

    if (validRects.empty()) return cv::Rect(0, 0, binary.cols, binary.rows);

    // Step 2: 가장 큰 컨투어 = Dominant (메인 인물)
    auto dominantIt = std::max_element(validRects.begin(), validRects.end(),
        [](const std::pair<double, cv::Rect>& a, const std::pair<double, cv::Rect>& b) {
            return a.first < b.first;
        });
    cv::Rect dominantRect = dominantIt->second;

    // Dominant bbox에 padding 적용 후 반환
    int padding = 10;
    dominantRect.x = std::max(0, dominantRect.x - padding);
    dominantRect.y = std::max(0, dominantRect.y - padding);
    dominantRect.width = std::min(binary.cols - dominantRect.x, dominantRect.width + 2 * padding);
    dominantRect.height = std::min(binary.rows - dominantRect.y, dominantRect.height + 2 * padding);

    return dominantRect;
}

cv::Mat ImageProcessor::Crop(const cv::Mat& image, const cv::Rect& roi) {
    if (image.empty()) return cv::Mat();
    cv::Rect validRoi = roi & cv::Rect(0, 0, image.cols, image.rows);
    if (validRoi.area() <= 0) return image;
    return image(validRoi).clone();
}
