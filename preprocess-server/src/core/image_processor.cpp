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
#include <spdlog/spdlog.h>

ImageProcessor::ImageProcessor() {
}

cv::Mat ImageProcessor::Load(const std::string& path) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) {
       spdlog::error("Failed to load image: {}", path);
    }
    return img;
}

cv::Mat ImageProcessor::Preprocess(const cv::Mat& input) {
    if (input.empty()) return cv::Mat();

    // 1단계: 그레이스케일 변환 및 노이즈 제거
    cv::Mat gray, blurred;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    // 개선된 사양에 따라 GaussianBlur (5x5) 적용
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    // 2단계: 적응형 이진화 (Adaptive Thresholding)
    cv::Mat binary;
    // adaptiveThreshold 파라미터: 11, 2
    // THRESH_BINARY_INV -> 배경(0, 검정), 선(255, 흰색)
    cv::adaptiveThreshold(blurred, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);

    // 모폴로지 (Close) - 끊어진 선 연결
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);

    // 3단계: 스마트 ROI 크롭 (0.1% 이상 크기의 모든 객체 포함)
    cv::Rect roi = GetContentROI(binary);

    // 그레이스케일 및 이진화 이미지 모두 크롭
    cv::Mat roi_gray = Crop(gray, roi);
    cv::Mat roi_binary = Crop(binary, roi);

    if (roi_gray.empty() || roi_binary.empty()) return cv::Mat();

    // 4단계: 레터박스 리사이즈 (종횡비 유지)
    // ----------------------------------------------------
    // 날카로운 엣지를 유지하기 위해 이진 마스크는 NEAREST 이웃 보간법으로 리사이즈
    float scale = static_cast<float>(kTargetSize) / std::max(roi_binary.cols, roi_binary.rows);
    int new_w = static_cast<int>(roi_binary.cols * scale);
    int new_h = static_cast<int>(roi_binary.rows * scale);

    cv::Mat resized_binary;
    cv::resize(roi_binary, resized_binary, cv::Size(new_w, new_h), 0, 0, cv::INTER_NEAREST);

    // 정사각형 캔버스 생성 (흰색 배경)
    cv::Mat canvas(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    int x_offset = (kTargetSize - new_w) / 2;
    int y_offset = (kTargetSize - new_h) / 2;
    resized_binary.copyTo(canvas(cv::Rect(x_offset, y_offset, new_w, new_h)));

    // 그레이 채널도 리사이즈 필요 (연속적인 톤에는 Linear 보간법이 더 좋음)
    cv::Mat resized_gray;
    cv::resize(roi_gray, resized_gray, cv::Size(new_w, new_h), 0, 0, cv::INTER_LINEAR);

    cv::Mat canvas_gray(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255)); // 흰색 배경
    resized_gray.copyTo(canvas_gray(cv::Rect(x_offset, y_offset, new_w, new_h)));

    // 5단계: 하이브리드 채널 구성
    // ----------------------------------------------------
    // R 채널: 그레이스케일 (필압/질감 정보)
    // 선은 검정, 배경은 흰색이 되도록 반전 -> 이진화 이미지와 매칭
    cv::Mat ch_gray;
    // 입력이 흰 종이/검은 펜인 경우 canvas_gray는 이미 흰 배경/검은 선이라고 가정
    // 하지만 원본 Denoise 결과는 가우시안 블러가 적용됨.
    // 입력이 흰 종이/검은 펜이었다면:
    //   roi_gray는 흰/검. canvas_gray도 흰/검.
    //   목표: 흰 배경, 검은 선.
    //   단순히 매칭되는지 확인.
    ch_gray = canvas_gray;

    // G 채널: 반전된 이진화 (형태/스트로크 정보)
    // canvas는 흰 배경 / 검은 선임 (BinarizeInv + Invert 로직에서?)
    // 잠깐, BinarizeFilter는 BinaryInv(검은 배경, 흰 선)를 생성했음.
    // 그 후 모폴로지 수행.
    // roi_binary는 BinaryInv였음.
    // canvas(0으로 채움) -> copyTo -> canvas는 검은 배경 / 흰 선인가?
    // 4단계를 다시 확인해보자.
    // canvas는 255(흰색)로 초기화됨.
    // resized_binary(BinaryInverted에서 옴)는 흰 선, 검은 배경임.
    // copyTo는 흰 배경 위에 흰 선을 놓나? 아님.
    // 투명 복사? 아님.
    // 계획을 준수해야 함: 최종 출력은 흰 배경 / 검은 선이어야 함.

    // 프롬프트의 명시적 요구사항을 따르자:
    // "Invert: 최종 결과물을 흰 배경/검은 선으로 반전"

    cv::Mat ch_inv_binary;
    cv::bitwise_not(canvas, ch_inv_binary); // 흰 선 -> 검은 선, 흰 배경 -> 검은 배경?
    // 패딩으로 흰색 캔버스를 사용했고, 이미지 내용이 검은 배경/흰 선이었다면...
    // 이를 반전시키면: 검은 패딩, 흰 배경/검은 선이 됨.
    // 로직이 복잡함. 단순화하자:
    // 1. 우리는 흰 배경 / 검은 선을 원함.
    // 2. ch_inv_binary가 바로 그것이어야 함.

    // 캔버스 초기화 재평가:
    // 흰 배경을 원한다면 255로 초기화.
    // 입력 `resized_binary`가 검은 배경/흰 선이라면.
    // 이를 맹목적으로 복사하면 흰 바탕에 검은 배경의 박스가 생김. 좋지 않음.

    // 수정: `resized_binary`를 먼저 반전시켜 흰 배경/검은 선으로 만듦.
    cv::Mat resized_bin_inv;
    cv::bitwise_not(resized_binary, resized_bin_inv);

    // 이제 캔버스는 255임. 그 위에 흰 배경/검은 선을 복사.
    resized_bin_inv.copyTo(canvas(cv::Rect(x_offset, y_offset, new_w, new_h)));
    ch_inv_binary = canvas;

    // B 채널: 거리 변환 (구조/위상 정보)
    cv::Mat ch_distance;
    // 거리 변환은 이진 이미지(흰 선)가 필요함.
    // `resized_binary`는 검은 배경/흰 선임. 완벽함.
    cv::matchTemplate(resized_binary, resized_binary, ch_distance, cv::TM_CCORR); // 자리 표시자? 아님.
    // 실제 거리 변환
    cv::distanceTransform(resized_binary, ch_distance, cv::DIST_L2, 5);
    cv::normalize(ch_distance, ch_distance, 0, 1.0, cv::NORM_MINMAX);
    ch_distance = ch_distance * 255;
    ch_distance.convertTo(ch_distance, CV_8U);

    // 거리 맵은 보통 객체의 중심이 밝음.
    // 흰 배경 / 검은 선 스타일을 원하는가? 아니면 히트맵?
    // 보통 거리 맵은 검은 배경에 흰 그라데이션임.
    // 다른 채널들과 맞추기 위해 반전시킴 (흰 배경, 어두운 중심).
    cv::bitwise_not(ch_distance, ch_distance);

    // 거리 맵에 대한 패딩 처리
    cv::Mat canvas_dist(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    ch_distance.copyTo(canvas_dist(cv::Rect(x_offset, y_offset, new_w, new_h)));

    // 병합
    std::vector<cv::Mat> channels = {ch_gray, ch_inv_binary, canvas_dist};
    cv::Mat merged;
    cv::merge(channels, merged);

    return merged;
}

bool ImageProcessor::Save(const cv::Mat& image, const std::string& path) {
    if (image.empty()) {
        return false;
    }
    return cv::imwrite(path, image);
}

// === Week 3: Advanced Preprocessing Implementation ===

#if 0
// [DEAD CODE] GrabCut Background Removal - Disabled
cv::Mat ImageProcessor::RemoveBackground(const cv::Mat& input, int iterCount) {
    if (input.empty()) {
        return cv::Mat();
    }

    // Initialize mask: edge 10% as probable background, center as probable foreground
    cv::Mat mask = cv::Mat::zeros(input.size(), CV_8UC1);

    // Define margin (10% of image size)
    int marginX = input.cols / 10;
    int marginY = input.rows / 10;

    // Set center region as probable foreground (GC_PR_FGD = 3)
    cv::Rect foregroundRect(marginX, marginY,
                            input.cols - 2 * marginX,
                            input.rows - 2 * marginY);
    mask(foregroundRect).setTo(cv::GC_PR_FGD);

    // Temporary arrays for GrabCut
    cv::Mat bgdModel, fgdModel;

    // Apply GrabCut with mask initialization
    // Deep Dive Note: iterCount affects speed vs quality
    // - iterCount=1: ~20-30ms, lower quality
    // - iterCount=3: ~40-60ms, balanced (default)
    // - iterCount=5: ~80-100ms, higher quality
    cv::grabCut(input, mask, cv::Rect(), bgdModel, fgdModel, iterCount, cv::GC_INIT_WITH_MASK);

    // Convert GrabCut mask to binary mask
    // GC_FGD=1, GC_PR_FGD=3 -> 255 (foreground)
    // GC_BGD=0, GC_PR_BGD=2 -> 0 (background)
    cv::Mat result;
    cv::compare(mask, cv::GC_PR_FGD, result, cv::CMP_EQ);
    cv::Mat fgdMask;
    cv::compare(mask, cv::GC_FGD, fgdMask, cv::CMP_EQ);
    result = result | fgdMask;

    return result;
}
#endif

cv::Mat ImageProcessor::DetectEdges(const cv::Mat& grayscale, double lowThreshold, double highThreshold) {
    if (grayscale.empty()) {
        return cv::Mat();
    }

    // Ensure input is grayscale
    cv::Mat input = grayscale;
    if (grayscale.channels() > 1) {
        cv::cvtColor(grayscale, input, cv::COLOR_BGR2GRAY);
    }

    cv::Mat edges;
    // Canny edge detection
    // apertureSize=3 (Sobel kernel size)
    // L2gradient=true for more accurate gradient magnitude
    cv::Canny(input, edges, lowThreshold, highThreshold, 3, true);

    return edges;
}

cv::Mat ImageProcessor::EnhanceContours(const cv::Mat& binary, int kernelSize) {
    if (binary.empty()) {
        return cv::Mat();
    }

    // Create structuring element for morphology
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(kernelSize, kernelSize)
    );

    cv::Mat result;
    // MORPH_CLOSE: Dilation followed by Erosion
    // Closes small gaps in contours while preserving shape
    cv::morphologyEx(binary, result, cv::MORPH_CLOSE, kernel);

    return result;
}

cv::Mat ImageProcessor::Binarize(const cv::Mat& grayscale) {
    if (grayscale.empty()) {
        return cv::Mat();
    }

    // Ensure input is grayscale
    cv::Mat input = grayscale;
    if (grayscale.channels() > 1) {
        cv::cvtColor(grayscale, input, cv::COLOR_BGR2GRAY);
    }

    cv::Mat result;
    // Adaptive thresholding: works better for varying lighting conditions
    // ADAPTIVE_THRESH_GAUSSIAN_C: weighted sum (Gaussian blur) of neighborhood
    // THRESH_BINARY_INV: dark objects on light background
    // blockSize=11: neighborhood size
    // C=2: constant subtracted from mean
    cv::adaptiveThreshold(input, result, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          11, 2);

    return result;
}

cv::Mat ImageProcessor::ResizeKeepingAspectRatio(const cv::Mat& input, int targetSize) {
    if (input.empty()) {
        return cv::Mat();
    }

    // Calculate scaling factor to fit within targetSize while preserving aspect ratio
    double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );

    int newWidth = static_cast<int>(input.cols * scale);
    int newHeight = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight));

    // Create canvas with black padding
    cv::Mat canvas(targetSize, targetSize, input.type(), cv::Scalar(0, 0, 0));

    // Center the resized image on the canvas
    int offsetX = (targetSize - newWidth) / 2;
    int offsetY = (targetSize - newHeight) / 2;
    resized.copyTo(canvas(cv::Rect(offsetX, offsetY, newWidth, newHeight)));

    return canvas;
}

cv::Rect ImageProcessor::GetContentROI(const cv::Mat& binary) {
    if (binary.empty()) return cv::Rect(0, 0, 0, 0);

    // Internal Morphology to close gaps before finding contours
    // This helps in detecting disconnected strokes as a single object group
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
        // Filter noise: ignore contours smaller than 0.1% of total area
        // Updated from 0.5% to 0.1% to preserve small objects like shoes/accessories
        if (cv::contourArea(contour) < totalArea * 0.001) continue;

        cv::Rect rect = cv::boundingRect(contour);
        if (first) {
            unionRect = rect;
            first = false;
        } else {
            unionRect |= rect; // Union of rectangles
        }
    }

    // Fallback: if all contours were filtered out as noise, use full image
    if (first) {
        return cv::Rect(0, 0, binary.cols, binary.rows);
    }
    
    // Add padding (10px or 2% of size)
    int padding = 10;
    unionRect.x = std::max(0, unionRect.x - padding);
    unionRect.y = std::max(0, unionRect.y - padding);
    unionRect.width = std::min(binary.cols - unionRect.x, unionRect.width + 2 * padding);
    unionRect.height = std::min(binary.rows - unionRect.y, unionRect.height + 2 * padding);

    return unionRect;
}

cv::Mat ImageProcessor::Crop(const cv::Mat& image, const cv::Rect& roi) {
    if (image.empty()) return cv::Mat();
    
    // Validate ROI intersection with image bounds
    cv::Rect validRoi = roi & cv::Rect(0, 0, image.cols, image.rows);
    if (validRoi.area() <= 0) return image;
    
    return image(validRoi).clone();
}
