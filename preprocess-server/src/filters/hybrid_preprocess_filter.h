#pragma once
#include "core/filter.h"

/**
 * @brief HybridPreprocessFilter - Advanced AI Preprocessing
 * 
 * Encapsulates the core logic of the Hybrid 3-Channel Strategy:
 * 1. Adaptive Thresholding & Morphology
 * 2. Smart ROI Calculation & Cropping
 * 3. 512x512 Letterbox Resizing
 * 4. Hybrid Channel Merge (Gray / InvBinary / Distance)
 */
class HybridPreprocessFilter : public IFilter {
public:
    HybridPreprocessFilter() = default;
    
    cv::Mat apply(const cv::Mat& input) const override;
    std::string name() const override { return "HybridPreprocess"; }

private:
    cv::Mat Binarize(const cv::Mat& input) const;
    cv::Rect GetContentROI(const cv::Mat& binary) const;
    cv::Mat Crop(const cv::Mat& image, const cv::Rect& roi) const;
};
