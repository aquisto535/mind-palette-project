#include "hybrid_preprocess_filter.h"
#include <vector>
#include <iostream>

const int kTargetSize = 512;

cv::Mat HybridPreprocessFilter::apply(const cv::Mat& input) const {
    if (input.empty()) return cv::Mat();
    
    // Input is expected to be Denoised Gray/BGR. Convert to Gray if needed.
    cv::Mat gray;
    if (input.channels() > 1) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    
    // Step 2: Adaptive Thresholding
    cv::Mat binary = Binarize(gray);
    
    // Morphology (Close)
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    
    // Step 3: Smart ROI Crop
    cv::Rect roi = GetContentROI(binary);
    
    cv::Mat roi_gray = Crop(gray, roi);
    cv::Mat roi_binary = Crop(binary, roi);
    
    if (roi_gray.empty() || roi_binary.empty()) return cv::Mat();

    // Step 4: Letterbox Resize
    float scale = std::min((float)kTargetSize / roi.width, (float)kTargetSize / roi.height);
    int new_w = (int)(roi.width * scale);
    int new_h = (int)(roi.height * scale);
    
    cv::Mat resized_gray, resized_binary;
    cv::resize(roi_gray, resized_gray, cv::Size(new_w, new_h), 0, 0, cv::INTER_AREA);
    cv::resize(roi_binary, resized_binary, cv::Size(new_w, new_h), 0, 0, cv::INTER_NEAREST);
    cv::resize(roi_binary, resized_binary, cv::Size(new_w, new_h)); // Final resize check

    // Step 5: Channel Construction
    int x_off = (kTargetSize - new_w) / 2;
    int y_off = (kTargetSize - new_h) / 2;
    cv::Rect target_roi(x_off, y_off, new_w, new_h);

    // R: Gray (Padding 255)
    cv::Mat ch_R(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    resized_gray.copyTo(ch_R(target_roi));

    // G: Inverted Binary (Padding 255)
    cv::Mat inverted_binary;
    cv::bitwise_not(resized_binary, inverted_binary);
    cv::Mat ch_G(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    inverted_binary.copyTo(ch_G(target_roi));
    
    // B: Distance (Padding 255)
    cv::Mat dist;
    cv::distanceTransform(resized_binary, dist, cv::DIST_L2, 5);
    cv::normalize(dist, dist, 0, 255, cv::NORM_MINMAX, CV_8UC1);
    cv::bitwise_not(dist, dist);
    
    cv::Mat ch_B(kTargetSize, kTargetSize, CV_8UC1, cv::Scalar(255));
    dist.copyTo(ch_B(target_roi));
    
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

    cv::Rect unionRect;
    bool first = true;
    double totalArea = binary.cols * binary.rows;

    for (const auto& contour : contours) {
        if (cv::contourArea(contour) < totalArea * 0.001) continue;

        cv::Rect rect = cv::boundingRect(contour);
        if (first) {
            unionRect = rect;
            first = false;
        } else {
            unionRect |= rect;
        }
    }

    if (first) return cv::Rect(0, 0, binary.cols, binary.rows);
    
    // Padding
    int padding = 10;
    unionRect.x = std::max(0, unionRect.x - padding);
    unionRect.y = std::max(0, unionRect.y - padding);
    unionRect.width = std::min(binary.cols - unionRect.x, unionRect.width + 2 * padding);
    unionRect.height = std::min(binary.rows - unionRect.y, unionRect.height + 2 * padding);

    return unionRect;
}

cv::Mat HybridPreprocessFilter::Crop(const cv::Mat& image, const cv::Rect& roi) const {
    if (image.empty()) return cv::Mat();
    cv::Rect validRoi = roi & cv::Rect(0, 0, image.cols, image.rows);
    if (validRoi.area() <= 0) return image;
    return image(validRoi).clone();
}
