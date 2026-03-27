#include "hybrid_preprocess_filter.h"
#include <vector>
#include <iostream>

const int kTargetSize = 512;

cv::Mat HybridPreprocessFilter::apply(const cv::Mat& input) const {
    if (input.empty()) return cv::Mat();
    
    // Step 1: Ensure Grayscale
    cv::Mat gray;
    if (input.channels() > 1) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    
    // Step 2: Adaptive Thresholding & Morphology
    cv::Mat binary = Binarize(gray);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    
    // Step 3: Smart ROI Crop
    cv::Rect roi = GetContentROI(binary);
    if (roi.area() <= 0) return cv::Mat();

    // Step 4 & 5: Prepare Letterbox and Construct Final Hybrid Image
    auto params = PrepareLetterbox(gray, binary, roi);
    return ConstructChannels(params);
}

HybridPreprocessFilter::LetterboxParams HybridPreprocessFilter::PrepareLetterbox(const cv::Mat& gray, const cv::Mat& binary, const cv::Rect& roi) const {
    cv::Mat roi_gray = Crop(gray, roi);
    cv::Mat roi_binary = Crop(binary, roi);
    
    float scale = std::min((float)kTargetSize / roi.width, (float)kTargetSize / roi.height);
    int new_w = (int)(roi.width * scale);
    int new_h = (int)(roi.height * scale);
    
    cv::Mat resized_gray, resized_binary;
    cv::resize(roi_gray, resized_gray, cv::Size(new_w, new_h), 0, 0, cv::INTER_AREA);
    cv::resize(roi_binary, resized_binary, cv::Size(new_w, new_h), 0, 0, cv::INTER_NEAREST);

    int x_off = (kTargetSize - new_w) / 2;
    int y_off = (kTargetSize - new_h) / 2;
    
    return {resized_gray, resized_binary, cv::Rect(x_off, y_off, new_w, new_h)};
}

cv::Mat HybridPreprocessFilter::ConstructChannels(const LetterboxParams& params) const {
    // R: Gray (Padding 255)
    cv::Mat ch_R(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    params.resized_gray.copyTo(ch_R(params.target_roi));

    // G: Inverted Binary (Padding 255)
    cv::Mat inverted_binary;
    cv::bitwise_not(params.resized_binary, inverted_binary);
    cv::Mat ch_G(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    inverted_binary.copyTo(ch_G(params.target_roi));
    
    // B: Distance (Padding 255)
    cv::Mat dist;
    cv::distanceTransform(params.resized_binary, dist, cv::DIST_L2, 5);
    cv::normalize(dist, dist, 0, 255, cv::NORM_MINMAX, CV_8UC1);
    cv::bitwise_not(dist, dist);
    
    cv::Mat ch_B(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    dist.copyTo(ch_B(params.target_roi));
    
    // Merge
    cv::Mat final;
    std::vector<cv::Mat> channels = {ch_R, ch_G, ch_B};
    cv::merge(channels, final);
    
    return final;
}

cv::Mat HybridPreprocessFilter::Binarize(const cv::Mat& input) const {
    cv::Mat result;
    cv::adaptiveThreshold(input, result, 255, 
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                          cv::THRESH_BINARY_INV, 
                          11, 2);
    return result;
}

cv::Rect HybridPreprocessFilter::GetContentROI(const cv::Mat& binary) const {
    if (binary.empty()) return cv::Rect(0, 0, 0, 0);

    // Internal Morphology for contour detection
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

cv::Mat HybridPreprocessFilter::Crop(const cv::Mat& image, const cv::Rect& roi) const {
    if (image.empty()) return cv::Mat();
    cv::Rect validRoi = roi & cv::Rect(0, 0, image.cols, image.rows);
    if (validRoi.area() <= 0) return image;
    return image(validRoi).clone();
}
