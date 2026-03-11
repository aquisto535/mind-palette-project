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

    // 1단계: 정규화된 그레이스케일 획득
    cv::Mat gray = NormalizeGrayscale(input);

    // 2단계: 적응형 이진화 및 노이즈 제거
    cv::Mat binary = ApplyAdaptiveBinarization(gray);

    // 3단계: 스마트 ROI 크롭
    cv::Rect roi = GetContentROI(binary);
    cv::Mat roi_gray = Crop(gray, roi);
    cv::Mat roi_binary = Crop(binary, roi);

    if (roi_gray.empty() || roi_binary.empty()) return cv::Mat();

    // 4단계: 레터박스 리사이즈 (각 채널 특성에 맞는 보간법 적용)
    auto res_bin = ApplyLetterboxWithMetrics(roi_binary, kTargetSize, cv::INTER_NEAREST, 255);
    auto res_gray = ApplyLetterboxWithMetrics(roi_gray, kTargetSize, cv::INTER_LINEAR, 255);

    // 5단계: 하이브리드 채널 구성
    // R: 그레이스케일, G: 반전 이진화(흰배경/검은선), B: 거리 변환
    cv::Mat ch_gray = res_gray.canvas;
    
    cv::Mat ch_inv_binary;
    cv::bitwise_not(res_bin.canvas, ch_inv_binary);
    
    cv::Mat ch_distance = GenerateDistanceMap(res_bin.canvas);

    // 병합 및 반환
    cv::Mat merged;
    cv::merge(std::vector<cv::Mat>{ch_gray, ch_inv_binary, ch_distance}, merged);
    return merged;
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
    cv::Mat binary;
    cv::adaptiveThreshold(input, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);
    
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    return binary;
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
    return ApplyAdaptiveBinarization(grayscale);
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

    cv::Rect unionRect;
    bool first = true;
    double totalArea = binary.cols * binary.rows;

    for (const auto& contour : contours) {
        if (cv::contourArea(contour) < totalArea * 0.001) continue;
        cv::Rect rect = cv::boundingRect(contour);
        if (first) { unionRect = rect; first = false; }
        else { unionRect |= rect; }
    }

    if (first) return cv::Rect(0, 0, binary.cols, binary.rows);
    
    int padding = 10;
    unionRect.x = std::max(0, unionRect.x - padding);
    unionRect.y = std::max(0, unionRect.y - padding);
    unionRect.width = std::min(binary.cols - unionRect.x, unionRect.width + 2 * padding);
    unionRect.height = std::min(binary.rows - unionRect.y, unionRect.height + 2 * padding);

    return unionRect;
}

cv::Mat ImageProcessor::Crop(const cv::Mat& image, const cv::Rect& roi) {
    if (image.empty()) return cv::Mat();
    cv::Rect validRoi = roi & cv::Rect(0, 0, image.cols, image.rows);
    if (validRoi.area() <= 0) return image;
    return image(validRoi).clone();
}
